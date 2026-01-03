#include <archivetools.h>
#include "systemfixturebase.h"


class CanaryRolloutTimeoutSystemTest : public SystemFixtureBase
{
protected:
    const int CANARY_DEVICE_COUNT = 10;
    void SetUp() override
    {
        SystemFixtureBase::SetUp();
        buildDockerImage(fs::path(PROJECT_ROOT_DIR) / "server" / "sim");
        runDockerContainer();
        waitForServerUp(5000);
    }
};

TEST_F(CanaryRolloutTimeoutSystemTest, AssignmentCleanupOnDeviceTimeout)
{
    const std::string version = "5.0.0";
    std::vector<int> deviceIds;

    for (int i = 0; i < CANARY_DEVICE_COUNT; ++i)
    {
        json body =
        {
            {"type", "Raspberry Pi 4"}, {"arch", "ARMv8"},
            {"platform", "Linux"}, {"pollingInterval", 1}
        };
        auto res = client->Post(REGISTER_PATH.c_str(), body.dump(), "application/json");
        ASSERT_EQ(res->status, 200);
        int id = json::parse(res->body)["id"].get<int>();
        deviceIds.push_back(id);

        std::string q = "?id=" + std::to_string(id) +
                        "&type=" + httplib::encode_uri("Raspberry Pi 4") +
                        "&platform=Linux&arch=ARMv8";
        client->Get((MANIFEST_PATH + q).c_str());
    }

    fs::path archiveDir = BASE_TEST_DIR / "timeout_archive_src";
    fs::remove_all(archiveDir);
    fs::create_directories(archiveDir);
    writeFile(archiveDir / "app_bin", "PAYLOAD");

    std::vector<uint8_t> archiveData;
    ASSERT_EQ(ArchiveTools::create_archive_from_paths({(archiveDir / "app_bin").string()}, archiveData), 0);

    std::string innerManifest = R"({
        "release": {
            "version": ")" + version + R"(",
            "type": "Raspberry Pi 4",
            "timestamp": "2025-11-12T10:23:00Z",
            "platform": "Linux",
            "arch": "ARMv8"
        },
        "files": [{"path": "/opt/app", "hash": {"algo":"sha256", "value":"v"}}]
    })";

    fs::path tmpdir = BASE_TEST_DIR / "timeout_upload_tmp";
    fs::create_directories(tmpdir);
    fs::path mFile = tmpdir / "manifest.json";
    writeFile(mFile, innerManifest);
    std::string sig = signAndBase64(mFile, tmpdir);
    json sigJson =
    {
        {"signature", {{"algo", "rsa-sha256"}, {"keyname", "main"}, {"value", sig}}}
    };

    auto resUpload = postMultipartUpload(UPLOAD_PATH, innerManifest, sigJson.dump(), archiveData,
                                         {{"canary","true"}, {"percentage","10"}, {"installationTime", "0"}});
    ASSERT_EQ(resUpload->status, 200);

    std::this_thread::sleep_for(std::chrono::seconds(2));

    int timedOutDeviceId = -1;
    for (int id : deviceIds)
    {
        std::string q = "?id=" + std::to_string(id) +
                        "&type=" + httplib::encode_uri("Raspberry Pi 4") +
                        "&platform=Linux&arch=ARMv8";
        auto res = client->Get((MANIFEST_PATH + q).c_str());
        if (res && res->status == 200)
        {
            auto res = client->Get((DOWNLOAD_PATH + q).c_str());
            timedOutDeviceId = id;
            break;
        }
    }

    ASSERT_NE(timedOutDeviceId, -1);

    {
        pqxx::connection conn("dbname=pco_test user=postgres host=127.0.0.1 port=5433");
        pqxx::work txn(conn);
        pqxx::result res = txn.exec("SELECT count(*) FROM release_assignments WHERE device_id = " + std::to_string(timedOutDeviceId));
        ASSERT_EQ(res[0][0].as<int>(), 1);
        txn.commit();
    }

    std::this_thread::sleep_for(std::chrono::seconds(80));

    {
        pqxx::connection conn("dbname=pco_test user=postgres host=127.0.0.1 port=5433");
        pqxx::work txn(conn);

        pqxx::result res = txn.exec("SELECT count(*) FROM release_assignments WHERE device_id = " + std::to_string(timedOutDeviceId));

        ASSERT_EQ(res[0][0].as<int>(), 0);

        txn.commit();
    }

    {
        pqxx::connection conn("dbname=pco_test user=postgres host=127.0.0.1 port=5433");
        pqxx::work txn(conn);

        pqxx::result res = txn.exec("SELECT is_canary, active FROM releases WHERE version = " + txn.quote(version));

        ASSERT_EQ(res.size(), 1);
        ASSERT_FALSE(res[0]["is_canary"].as<bool>());
        ASSERT_FALSE(res[0]["active"].as<bool>());

        txn.commit();
    }
}
