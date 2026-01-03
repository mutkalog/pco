#include "core/deviceconfig.h"
#include "core/http/uploadcontroller.h"
#include "core/service/uploadservice.h"
#include "integrationbasefixture.h"
#include "core/rolloutsupervisor.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <pqxx/pqxx>
#include <filesystem>

class UploadIntegrationTest : public IntegrationBaseFixture<UploadController>
{
protected:
    std::unique_ptr<RolloutSupervisor> supervisor;

    void SetUp() override
    {
        IntegrationBaseFixture::SetUp();

        auto realService = std::make_unique<UploadService>(BASE_TEST_DIR / "buffer", BASE_TEST_DIR / "storage");
        controller = std::make_unique<UploadController>(sc, std::move(realService));

        supervisor = std::make_unique<RolloutSupervisor>(sc);
    }

    void callProcess() { supervisor->process(); }

    std::string makeRawJson(const std::string& version, const std::string& type,
                            const std::string& platform, const std::string& arch)
    {
        std::string innerManifest = R"({
            "release": {
                "version": ")" + version + R"(",
                "type": ")" + type + R"(",
                "timestamp": "2025-11-12T10:23:00Z",
                "platform": ")" + platform + R"(",
                "arch": ")" + arch + R"("
            },
            "files": [
                {"path": "/opt/app", "hash": {"algo": "sha256", "value": "hash123"}}
            ]
        })";

        std::string innerSignature = R"({
            "signature": {
                "algo": "rsa-sha256",
                "keyname": "test-key",
                "value": "base64-dummy-signature-data"
            }
        })";

        json root;
        root["manifest"] = innerManifest;
        root["signature"] = innerSignature;
        return root.dump();
    }

    httplib::Result postMultipart(
            httplib::Client& client,
            const std::string& path,
            const std::string& manifest = R"({"version":"1.0"})",
            const std::string& archive  = "binary_data",
            const httplib::Params& params = {})
    {
        httplib::UploadFormDataItems items;

        items.push_back({
            "manifest",
            manifest,
            "manifest.json",
            "application/json"
        });

        items.push_back({
            "archive",
            archive,
            "update.tar.gz",
            "application/gzip"
        });

        std::string full_path = path;
        if (!params.empty())
        {
            full_path += "?";
            bool first = true;
            for (const auto& [k, v] : params)
            {
                if (!first) full_path += "&";
                full_path += k + "=" + v;
                first = false;
            }
        }

        return client.Post(full_path, items);
    }

    void createDevice(const std::string& type, const std::string& platform, const std::string& arch)
    {
        pqxx::connection conn("dbname=" + testDbname + " user=postgres host=127.0.0.1 port=5433");
        pqxx::work txn(conn);

        txn.exec("INSERT INTO devices (device_type, platform, arch, last_seen, poling_interval) VALUES (" +
                 txn.quote(type) + ", " + txn.quote(platform) + ", " + txn.quote(arch) + ", now(), 10)").no_rows();

        txn.commit();
    }

    void simulateDeviceSuccess(uint64_t releaseId)
    {
        pqxx::connection conn("dbname=" + testDbname + " user=postgres host=127.0.0.1 port=5433");
        pqxx::work txn(conn);

        txn.exec("UPDATE release_assignments SET status = 'success' WHERE release_id = " +
                 std::to_string(releaseId)).no_rows();

        txn.commit();
    }

    void simulateDeviceFailure(uint64_t releaseId)
    {
        pqxx::connection conn("dbname=" + testDbname + " user=postgres host=127.0.0.1 port=5433");
        pqxx::work txn(conn);

        txn.exec("UPDATE release_assignments SET status = 'failed' WHERE release_id = " +
                 std::to_string(releaseId)).no_rows();

        txn.commit();
    }

    std::pair<bool, int> getReleaseStatus(uint64_t id)
    {
        pqxx::connection conn("dbname=" + testDbname + " user=postgres host=127.0.0.1 port=5433");
        pqxx::work txn(conn);
        auto res = txn.exec("SELECT is_canary, canary_percent FROM releases WHERE id = " + std::to_string(id));
        if (res.empty()) return {false, -1};
        return {res[0][0].as<bool>(), res[0][1].as<int>()};
    }

    int getAssignmentCount(uint64_t releaseId)
    {
        pqxx::connection conn("dbname=" + testDbname + " user=postgres host=127.0.0.1 port=5433");
        pqxx::work txn(conn);
        auto res = txn.exec("SELECT count(*) FROM release_assignments WHERE release_id = " + std::to_string(releaseId));
        return res[0][0].as<int>();
    }
};


TEST_F(UploadIntegrationTest, StandardUploadFlow)
{
    startServer(20001);
    std::string rawJson = makeRawJson("1.2.0", "Raspberry Pi 4", "Linux", "ARMv8");

    auto res = postMultipart(*client, "/upload", rawJson, "bin_data", {{"canary", "false"}});
    ASSERT_EQ(res->status, 200);

    callProcess();

    auto& rollouts = sc->staging.rollouts;
    std::lock_guard<std::mutex> lg(rollouts.mtx);
    ASSERT_TRUE(rollouts.releaseToInfoMap.empty());
}


TEST_F(UploadIntegrationTest, CanaryUploadCorrectlyInitialized)
{
    startServer(19602);
    std::string version = "1.2.0";
    std::string manifest = makeRawJson(version, "Raspberry Pi 4", "Linux", "ARMv8");

    auto res_http = postMultipart(*client, "/upload", manifest, "bin", {{"canary", "true"}, {"percentage", "10"}});
    ASSERT_EQ(res_http->status, 200);

    pqxx::connection conn("dbname=" + testDbname + " user=postgres host=127.0.0.1 port=5433");
    pqxx::work txn(conn);
    auto res = txn.exec("SELECT is_canary, canary_percent FROM releases WHERE version=" + txn.quote(version));

    ASSERT_FALSE(res.empty());
    ASSERT_EQ(res[0]["is_canary"].as<bool>(), true);
    ASSERT_EQ(res[0]["canary_percent"].as<int>(), 10);

    auto& rollouts = sc->staging.rollouts;
    std::lock_guard<std::mutex> lg(rollouts.mtx);
    ASSERT_FALSE(rollouts.releaseToInfoMap.empty());

    uint64_t relId = txn.exec("SELECT id FROM releases WHERE version=" + txn.quote(version))[0][0].as<uint64_t>();
    ASSERT_EQ(rollouts.releaseToInfoMap.at(relId).inRolloutPercentage, 10);
}


TEST_F(UploadIntegrationTest, CanaryCycleReaches100PercentAndCommits)
{
    startServer(19603);
    std::string version = "2.0.0";
    for (int i = 0; i < 10; ++i)
        createDevice("Raspberry Pi 4", "Linux", "ARMv8");

    std::string manifest = makeRawJson(version, "Raspberry Pi 4", "Linux", "ARMv8");

    postMultipart(*client, "/upload", manifest, "bin", {{"canary", "true"}, {"percentage", "25"}});

    pqxx::connection conn("dbname=" + testDbname + " user=postgres host=127.0.0.1 port=5433");
    pqxx::work txn(conn);
    uint64_t relId = txn.exec("SELECT id FROM releases WHERE version=" + txn.quote(version))[0][0].as<uint64_t>();
    txn.commit();

    callProcess();
    ASSERT_GT(getAssignmentCount(relId), 0);

    simulateDeviceSuccess(relId);
    callProcess();
    EXPECT_EQ(getReleaseStatus(relId).second, 50);

    simulateDeviceSuccess(relId);
    callProcess();
    EXPECT_EQ(getReleaseStatus(relId).second, 100);

    simulateDeviceSuccess(relId);
    callProcess();

    auto finalStatus = getReleaseStatus(relId);
    EXPECT_FALSE(finalStatus.first);
    EXPECT_EQ(finalStatus.second, 100);

    auto& rollouts = sc->staging.rollouts;
    std::lock_guard<std::mutex> lg(rollouts.mtx);
    EXPECT_TRUE(rollouts.releaseToInfoMap.empty());
}


TEST_F(UploadIntegrationTest, CanaryFailureInvalidatesRelease)
{
    startServer(19604);
    std::string version = "3.0.0";
    createDevice("Raspberry Pi 4", "Linux", "ARMv8");

    std::string manifest = makeRawJson(version, "Raspberry Pi 4", "Linux", "ARMv8");

    postMultipart(*client, "/upload", manifest, "bin", {{"canary", "true"}, {"percentage", "50"}});

    pqxx::connection conn("dbname=" + testDbname + " user=postgres host=127.0.0.1 port=5433");
    pqxx::work txn(conn);
    uint64_t relId = txn.exec("SELECT id FROM releases WHERE version=" + txn.quote(version))[0][0].as<uint64_t>();
    txn.commit();

    callProcess();
    simulateDeviceFailure(relId);
    callProcess();

    pqxx::work txn2(conn);
    auto res = txn2.exec("SELECT active, is_canary FROM releases WHERE id=" + std::to_string(relId));
    EXPECT_FALSE(res[0]["active"].as<bool>());
    EXPECT_FALSE(res[0]["is_canary"].as<bool>());

    auto count = txn2.exec("SELECT count(*) FROM release_assignments WHERE release_id=" + std::to_string(relId));
    EXPECT_EQ(count[0][0].as<int>(), 0);
}


TEST_F(UploadIntegrationTest, MissingArchiveReturnsBadRequest)
{
    startServer(19605);

    httplib::UploadFormDataItems items;
    items.push_back({"manifest", "{}", "m.json", "application/json"});

    auto res = client->Post("/upload?canary=true", items);
    EXPECT_EQ(res->status, 400);
}


TEST_F(UploadIntegrationTest, DuplicateVersionReturnsConflict)
{
    startServer(19606);
    std::string version = "4.0.0";
    std::string manifest = makeRawJson(version, "Raspberry Pi 4", "Linux", "ARMv8");

    auto res1 = postMultipart(*client, "/upload", manifest, "bin");
    EXPECT_EQ(res1->status, 200);

    auto res2 = postMultipart(*client, "/upload", manifest, "bin");
    EXPECT_EQ(res2->status, 409);
}


TEST_F(UploadIntegrationTest, InactiveDeviceStopsRollout)
{
    startServer(19607);
    std::string version = "5.0.0";
    createDevice("Raspberry Pi 4", "Linux", "ARMv8");

    std::string manifest = makeRawJson(version, "Raspberry Pi 4", "Linux", "ARMv8");

    postMultipart(*client, "/upload", manifest, "bin", {{"canary", "true"}, {"percentage", "100"}});

    pqxx::connection conn("dbname=" + testDbname + " user=postgres host=127.0.0.1 port=5433");
    pqxx::work txn(conn);
    uint64_t relId = txn.exec("SELECT id FROM releases WHERE version=" + txn.quote(version))[0][0].as<uint64_t>();
    txn.commit();

    callProcess();

    pqxx::work txn2(conn);
    txn2.exec("UPDATE devices SET last_seen = now() - interval '2 hour'");
    txn2.commit();

    callProcess();

    auto status = getReleaseStatus(relId);
    EXPECT_FALSE(status.first);
}
