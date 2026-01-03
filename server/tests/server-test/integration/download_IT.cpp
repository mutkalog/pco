#include "core/http/downloadcontroller.h"
#include "core/service/downloadservice.h"
#include "core/updatesupervisor.h"
#include "integrationbasefixture.h"
#include "mocks/archivetools_mock.h"
#include "mocks/cryptoutils_mock.h"

#include <archive.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <pqxx/pqxx>

using namespace testing;

class DownloadIntegrationTest : public IntegrationBaseFixture<DownloadController>
{
protected:
    MockArchiveTools* mockArchivePtr = nullptr;
    MockCryptoUtils*  mockCryptoPtr  = nullptr;
    std::unique_ptr<UpdateSupervisor> supervisor;

    void SetUp() override
    {
        IntegrationBaseFixture::SetUp();

        mockArchivePtr = static_cast<MockArchiveTools*>(sc->archiveTools.get());
        mockCryptoPtr  = static_cast<MockCryptoUtils*>(sc->cryptoUtils.get());

        ON_CALL(*mockArchivePtr, create_archive_from_paths(_, _))
            .WillByDefault([](const std::vector<std::string>&, std::vector<uint8_t>& out) {
                const std::string sample = "GZIPDATA";
                out.assign(sample.begin(), sample.end());
                return ARCHIVE_OK;
            });

        auto realService = std::make_unique<DownloadService>();
        controller = std::make_unique<DownloadController>(sc, std::move(realService));

        supervisor = std::make_unique<UpdateSupervisor>(sc);
    }

    void callProcess() { supervisor->process(); }

    int64_t insertRelease(const std::string& manifestRaw,
                          const std::string& signatureRaw,
                          const std::string& version,
                          const std::string& deviceType,
                          const std::string& platform,
                          const std::string& arch,
                          const std::vector<std::string>& filePaths,
                          bool active = true,
                          bool isCanary = false,
                          int canaryPercent = 0,
                          int installationTime = 60)
    {
        std::string filePathsArray = "{";
        for (size_t i = 0; i < filePaths.size(); ++i)
        {
            if (i > 0) filePathsArray += ",";
            filePathsArray += "\"" + filePaths[i] + "\"";
        }
        filePathsArray += "}";

        auto conn = pqxx::connection("dbname=" + testDbname + " user=postgres host=127.0.0.1 port=5433");
        pqxx::work txn(conn);
        pqxx::params params;
        params.append(manifestRaw);
        params.append(signatureRaw);
        params.append(version);
        params.append(deviceType);
        params.append(platform);
        params.append(arch);
        params.append(filePathsArray);
        params.append(active);
        params.append(isCanary);
        params.append(canaryPercent);
        params.append(installationTime);

        auto result = txn.exec(
            "INSERT INTO releases "
            "(manifest_raw, signature_raw, version, device_type, platform, arch, "
            "file_paths, active, is_canary, canary_percent, installation_time) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11) "
            "RETURNING id",
            params
        );
        txn.commit();
        return result[0][0].as<int64_t>();
    }

    int64_t insertDevice(const std::string& deviceType,
                         const std::string& platform,
                         const std::string& arch,
                         int pollingInterval = 300)
    {
        auto conn = pqxx::connection("dbname=" + testDbname + " user=postgres host=127.0.0.1 port=5433");
        pqxx::work txn(conn);
        pqxx::params params;
        params.append(deviceType);
        params.append(platform);
        params.append(arch);
        params.append(pollingInterval);

        auto result = txn.exec(
            "INSERT INTO devices (device_type, platform, arch, poling_interval) "
            "VALUES ($1, $2, $3, $4) RETURNING id",
            params
        );
        txn.commit();
        return result[0][0].as<int64_t>();
    }

    void insertReleaseAssignment(int64_t releaseId, int64_t deviceId,
                                 const std::string& status = "pending")
    {
        auto conn = pqxx::connection("dbname=" + testDbname + " user=postgres host=127.0.0.1 port=5433");
        pqxx::work txn(conn);
        pqxx::params params;
        params.append(releaseId);
        params.append(deviceId);
        params.append(status);

        txn.exec(
            "INSERT INTO release_assignments (release_id, device_id, status) "
            "VALUES ($1, $2, $3)",
            params
        );
        txn.commit();
    }

    bool hasUpdateInfo(uint64_t devId)
    {
        auto& updates = sc->staging.updates;
        std::lock_guard<std::mutex> lg(updates.mtx);
        return updates.devToReleaseMap.find(devId) != updates.devToReleaseMap.end();
    }

    UpdateInfo getUpdateInfo(uint64_t devId)
    {
        auto& updates = sc->staging.updates;
        std::lock_guard<std::mutex> lg(updates.mtx);
        return updates.devToReleaseMap.at(devId);
    }

    void clearUpdateInfo()
    {
        auto& updates = sc->staging.updates;
        std::lock_guard<std::mutex> lg(updates.mtx);
        updates.devToReleaseMap.clear();
    }

    int getReportsCountForDevice(uint64_t devId)
    {
        pqxx::connection conn("dbname=" + testDbname + " user=postgres host=127.0.0.1 port=5433");
        pqxx::work txn(conn);
        auto r = txn.exec("SELECT count(*) FROM reports WHERE device_id = " + std::to_string(devId));
        return r[0][0].as<int>();
    }

    int getAssignmentStatusCount(uint64_t releaseId, const std::string& status)
    {
        pqxx::connection conn("dbname=" + testDbname + " user=postgres host=127.0.0.1 port=5433");
        pqxx::work txn(conn);
        auto r = txn.exec("SELECT count(*) FROM release_assignments WHERE release_id = " + std::to_string(releaseId) +
                          " AND status = " + txn.quote(status));
        return r[0][0].as<int>();
    }

    uint64_t getDeviceInstalledRelease(uint64_t devId)
    {
        pqxx::connection conn("dbname=" + testDbname + " user=postgres host=127.0.0.1 port=5433");
        pqxx::work txn(conn);
        auto r = txn.exec("SELECT release_id FROM devices WHERE id = " + std::to_string(devId));
        if (r.empty()) return 0;
        return r[0][0].as<uint64_t>();
    }

    std::string buildDownloadPath(uint32_t id,
                                  const std::string& type,
                                  const std::string& platform,
                                  const std::string& arch)
    {
        return "/download?id=" + std::to_string(id) +
               "&type=" + type +
               "&platform=" + platform +
               "&arch=" + arch;
    }
};

TEST_F(DownloadIntegrationTest, DownloadThenSupervisorTimeoutProducesReportAndMarksAssignmentFailed)
{
    startServer(20100);

    auto devId = insertDevice("sensor", "linux", "arm64");
    auto relId = insertRelease(R"({"version":"1.0.23"})", "sig", "1.0.23", "sensor", "linux", "arm64",
                               {"/opt/bin"}, true, false, 0, 0);

    insertReleaseAssignment(relId, devId);

    EXPECT_CALL(*mockArchivePtr, create_archive_from_paths(_, _))
        .WillOnce([](const std::vector<std::string>&, std::vector<uint8_t>& archive) {
            archive = {0xAA};
            return ARCHIVE_OK;
        });

    auto res = client->Get(buildDownloadPath(static_cast<uint32_t>(devId), "sensor", "linux", "arm64"));
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, httplib::OK_200);
    ASSERT_TRUE(hasUpdateInfo(devId));

    {
        auto& updates = sc->staging.updates;
        std::lock_guard<std::mutex> lg(updates.mtx);
        auto it = updates.devToReleaseMap.find(devId);
        ASSERT_TRUE(it != updates.devToReleaseMap.end());
        const_cast<decltype(it->second)&>(it->second).expireTime =
            std::chrono::steady_clock::now() - std::chrono::minutes(1);
    }

    callProcess();

    EXPECT_GT(getReportsCountForDevice(devId), 0);
    EXPECT_GT(getAssignmentStatusCount(relId, "failed"), 0);
}

TEST_F(DownloadIntegrationTest, DownloadThenSupervisorProcessesSuccessfulDeviceReportAndCommitsInstalledRelease)
{
    startServer(20101);

    auto devId = insertDevice("sensor", "linux", "arm64");
    auto relId = insertRelease(R"({"version":"1.0.24"})", "sig", "1.0.24", "sensor", "linux", "arm64",
                               {"/opt/bin"}, true, false, 0, 0);
    insertReleaseAssignment(relId, devId);

    EXPECT_CALL(*mockArchivePtr, create_archive_from_paths(_, _))
        .WillOnce([](const std::vector<std::string>&, std::vector<uint8_t>& archive) {
            archive = {0xBB};
            return ARCHIVE_OK;
        });

    auto res = client->Get(buildDownloadPath(static_cast<uint32_t>(devId), "sensor", "linux", "arm64"));
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, httplib::OK_200);
    ASSERT_TRUE(hasUpdateInfo(devId));

    {
        auto& updates = sc->staging.updates;
        std::lock_guard<std::mutex> lg(updates.mtx);
        auto it = updates.devToReleaseMap.find(devId);
        ASSERT_TRUE(it != updates.devToReleaseMap.end());
        auto& info = const_cast<decltype(it->second)&>(it->second);
        info.finished = true;
        info.status = 0;
        info.report = R"({"result":"ok"})";
    }

    callProcess();

    EXPECT_EQ(getDeviceInstalledRelease(devId), relId);
    EXPECT_GT(getReportsCountForDevice(devId), 0);
    EXPECT_GT(getAssignmentStatusCount(relId, "success"), 0);
}

TEST_F(DownloadIntegrationTest, MultipleDownloadsThenSupervisorProcessesAllExpiredUpdates)
{
    startServer(20102);

    auto relId = insertRelease(R"({"version":"1.2.3"})", "sig", "1.2.3", "app", "linux", "x64",
                               {"/app/b"});
    std::vector<int64_t> devs;
    for (int i = 0; i < 3; ++i)
    {
        auto d = insertDevice("app", "linux", "x64");
        devs.push_back(d);
        insertReleaseAssignment(relId, d);
    }

    EXPECT_CALL(*mockArchivePtr, create_archive_from_paths(_, _))
        .Times(3)
        .WillRepeatedly([](const std::vector<std::string>&, std::vector<uint8_t>& archive) {
            archive = {0x01};
            return ARCHIVE_OK;
        });

    for (auto d : devs)
    {
        auto r = client->Get(buildDownloadPath(static_cast<uint32_t>(d), "app", "linux", "x64"));
        ASSERT_TRUE(r);
        EXPECT_EQ(r->status, httplib::OK_200);
    }

    {
        auto& updates = sc->staging.updates;
        std::lock_guard<std::mutex> lg(updates.mtx);
        for (auto d : devs)
        {
            auto it = updates.devToReleaseMap.find(d);
            ASSERT_TRUE(it != updates.devToReleaseMap.end());
            const_cast<decltype(it->second)&>(it->second).expireTime =
                std::chrono::steady_clock::now() - std::chrono::minutes(1);
        }
    }

    callProcess();

    for (auto d : devs)
    {
        EXPECT_GT(getReportsCountForDevice(d), 0);
    }
    EXPECT_GT(getAssignmentStatusCount(relId, "failed"), 0);
}

TEST_F(DownloadIntegrationTest, SuccessfulDownloadWithCanaryAssignmentPassesUpdateInfo)
{
    startServer(20020);

    auto devId = insertDevice("sensor", "linux", "arm64");
    auto relId = insertRelease(R"({"version":"1.0"})", "sig", "1.0", "sensor", "linux", "arm64",
                               {"/opt/bin/app"}, true, true, 10);

    insertReleaseAssignment(relId, devId);

    std::vector<uint8_t> expected = {0x1F, 0x8B};
    EXPECT_CALL(*mockArchivePtr, create_archive_from_paths(_, _))
        .WillOnce([&expected](const std::vector<std::string>&, std::vector<uint8_t>& archive) {
            archive = expected;
            return ARCHIVE_OK;
        });

    auto res = client->Get(buildDownloadPath(static_cast<uint32_t>(devId), "sensor", "linux", "arm64"));
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, httplib::OK_200);
    EXPECT_EQ(res->get_header_value("Content-Type"), "application/gzip");
    EXPECT_FALSE(res->body.empty());

    EXPECT_TRUE(hasUpdateInfo(devId));
    auto info = getUpdateInfo(devId);
    EXPECT_EQ(info.releaseId, static_cast<uint64_t>(relId));
}
