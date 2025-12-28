#include <archive.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>

#include "core/service/downloadservice.h"
#include "core/database.h"

#include "mocks/archivetools_mock.h"
#include "mocks/cryptoutils_mock.h"
#include "servicefixturebase.h"

using json = nlohmann::ordered_json;
using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;

class DownloadServiceTest : public ServiceFixtureBase
{
protected:
    MockArchiveTools* mockArchivePtr;
    MockCryptoUtils* mockCryptoPtr;
    std::unique_ptr<DownloadService> service;

    void SetUp() override
    {
        Database::instance(testDbname, "postgres", "127.0.0.1", "5433");

        try {
            conn = std::make_unique<pqxx::connection>(connString);
        } catch (const std::exception& e) {
            FAIL() << "Could not connect to test database: " << e.what();
        }

        clearTables();

        auto mockArchive = std::make_unique<NiceMock<MockArchiveTools>>();
        auto mockCrypto  = std::make_unique<NiceMock<MockCryptoUtils>>();

        mockArchivePtr = mockArchive.get();
        mockCryptoPtr  = mockCrypto.get();

        sc = std::make_shared<ServerContext>(
            std::move(mockCrypto),
            std::move(mockArchive)
        );

        service = std::make_unique<DownloadService>();
    }

    void TearDown() override
    {
        clearTables();
    }

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

        pqxx::work txn(*conn);
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
        pqxx::work txn(*conn);
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
        pqxx::work txn(*conn);
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
};


TEST_F(DownloadServiceTest, GetArchiveNoDeviceThrows)
{
    uint32_t nonExistentDeviceId = 99999;

    EXPECT_THROW(
        service->getArchive(sc, nonExistentDeviceId, "app", "linux", "x64"),
        pqxx::sql_error
    );
}


TEST_F(DownloadServiceTest, GetArchiveNoReleaseThrows)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    EXPECT_THROW(
        service->getArchive(sc, deviceId, "app", "linux", "x64"),
        pqxx::sql_error
    );
}

TEST_F(DownloadServiceTest, GetArchiveNoMatchingReleaseThrows)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    insertRelease(
        R"({"version":"1.0.0"})", "sig123", "1.0.0",
        "app", "windows", "x64",
        {"/path/to/file.bin"}
    );

    EXPECT_THROW(
        service->getArchive(sc, deviceId, "app", "linux", "x64"),
        pqxx::sql_error
    );
}


TEST_F(DownloadServiceTest, GetArchiveFallbackReleaseReturnsArchive)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    insertRelease(
        R"({"version":"2.0.0"})", "signature", "2.0.0",
        "app", "linux", "x64",
        {"/path/to/binary.bin", "/path/to/config.json"}
    );

    std::vector<uint8_t> expectedArchive = {0x50, 0x4B, 0x03, 0x04};
    EXPECT_CALL(*mockArchivePtr, create_archive_from_paths(_, _))
        .WillOnce([&expectedArchive](const std::vector<std::string>&,
                                     std::vector<uint8_t>& archive) {
            archive = expectedArchive;
            return ARCHIVE_OK;
        });

    auto result = service->getArchive(sc, deviceId, "app", "linux", "x64");

    EXPECT_EQ(result, expectedArchive);
}


TEST_F(DownloadServiceTest, GetArchiveCanaryReleaseReturnsArchive)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    int64_t canaryReleaseId = insertRelease(
        R"({"version":"3.0.0-canary"})", "canary_sig", "3.0.0-canary",
        "app", "linux", "x64",
        {"/canary/path/file.bin"},
        true, true, 10
    );

    insertReleaseAssignment(canaryReleaseId, deviceId);

    std::vector<uint8_t> expectedArchive = {0xCA, 0xAA, 0xAB};
    EXPECT_CALL(*mockArchivePtr, create_archive_from_paths(_, _))
        .WillOnce([&expectedArchive](const std::vector<std::string>&,
                                     std::vector<uint8_t>& archive) {
            archive = expectedArchive;
            return ARCHIVE_OK;
        });

    auto result = service->getArchive(sc, deviceId, "app", "linux", "x64");

    EXPECT_EQ(result, expectedArchive);
}


TEST_F(DownloadServiceTest, GetArchiveCanaryPriorityOverFallback)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    insertRelease(
        R"({"version":"1.0.0"})", "fallback_sig", "1.0.0",
        "app", "linux", "x64",
        {"/fallback/file.bin"}
    );

    int64_t canaryReleaseId = insertRelease(
        R"({"version":"2.0.0-canary"})", "canary_sig", "2.0.0-canary",
        "app", "linux", "x64",
        {"/canary/file.bin"},
        true, true, 10
    );

    insertReleaseAssignment(canaryReleaseId, deviceId);

    std::vector<std::string> capturedPaths;
    EXPECT_CALL(*mockArchivePtr, create_archive_from_paths(_, _))
        .WillOnce([&capturedPaths](const std::vector<std::string>& paths,
                                   std::vector<uint8_t>& archive) {
            capturedPaths = paths;
            archive = {0x01, 0x02};
            return ARCHIVE_OK;
        });

    service->getArchive(sc, deviceId, "app", "linux", "x64");

    ASSERT_EQ(capturedPaths.size(), 1);
    EXPECT_EQ(capturedPaths[0], "/canary/file.bin");
}


TEST_F(DownloadServiceTest, GetArchiveEmptyFilePathsThrows)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    insertRelease(
        R"({"version":"1.0.0"})", "signature", "1.0.0",
        "app", "linux", "x64",
        {}
    );

    EXPECT_THROW(
        service->getArchive(sc, deviceId, "app", "linux", "x64"),
        std::system_error
    );
}


TEST_F(DownloadServiceTest, GetArchiveArchiveCreationFailsThrows)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    insertRelease(
        R"({"version":"1.0.0"})", "signature", "1.0.0",
        "app", "linux", "x64",
        {"/path/to/file.bin"}
    );

    EXPECT_CALL(*mockArchivePtr, create_archive_from_paths(_, _))
        .WillOnce(Return(ARCHIVE_FATAL));

    EXPECT_THROW(
        service->getArchive(sc, deviceId, "app", "linux", "x64"),
        std::runtime_error
    );
}


TEST_F(DownloadServiceTest, GetArchivePassesUpdateInfoOnSuccess)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    int64_t releaseId = insertRelease(
        R"({"version":"1.0.0"})", "signature", "1.0.0",
        "app", "linux", "x64",
        {"/path/to/file.bin"},
        true, false, 0, 30
    );

    EXPECT_CALL(*mockArchivePtr, create_archive_from_paths(_, _))
        .WillOnce([](const std::vector<std::string>&,
                     std::vector<uint8_t>& archive) {
            archive = {0x01};
            return ARCHIVE_OK;
        });

    service->getArchive(sc, deviceId, "app", "linux", "x64");

    EXPECT_TRUE(hasUpdateInfo(deviceId));

    auto info = getUpdateInfo(deviceId);
    EXPECT_EQ(info.releaseId, static_cast<uint64_t>(releaseId));
    EXPECT_NE(info.status, 0);
}


TEST_F(DownloadServiceTest, GetArchivePassesUpdateInfoOnArchiveFailure)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    insertRelease(
        R"({"version":"1.0.0"})", "signature", "1.0.0",
        "app", "linux", "x64",
        {"/path/to/file.bin"}
    );

    EXPECT_CALL(*mockArchivePtr, create_archive_from_paths(_, _))
        .WillOnce(Return(ARCHIVE_FATAL));

    EXPECT_THROW(
        service->getArchive(sc, deviceId, "app", "linux", "x64"),
        std::runtime_error
    );

    EXPECT_TRUE(hasUpdateInfo(deviceId));
}


TEST_F(DownloadServiceTest, GetArchivePassesFailInfoWhenNoRelease)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    ASSERT_THROW(
        service->getArchive(sc, deviceId, "app", "linux", "x64"),
        pqxx::sql_error
    );

    EXPECT_TRUE(hasUpdateInfo(deviceId));

    auto info = getUpdateInfo(deviceId);
    EXPECT_EQ(info.releaseId, static_cast<uint64_t>(-1));
}


TEST_F(DownloadServiceTest, GetArchiveInactiveReleaseNotReturned)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    insertRelease(
        R"({"version":"1.0.0"})", "signature", "1.0.0",
        "app", "linux", "x64",
        {"/path/to/file.bin"},
        false, false
    );

    EXPECT_THROW(
        service->getArchive(sc, deviceId, "app", "linux", "x64"),
        pqxx::sql_error
    );
}


TEST_F(DownloadServiceTest, GetArchiveCanaryWithoutAssignmentNotReturned)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    insertRelease(
        R"({"version":"2.0.0-canary"})", "canary_sig", "2.0.0-canary",
        "app", "linux", "x64",
        {"/canary/file.bin"},
        true, true, 10
    );

    EXPECT_THROW(
        service->getArchive(sc, deviceId, "app", "linux", "x64"),
        pqxx::sql_error
    );
}

TEST_F(DownloadServiceTest, GetArchiveDifferentArchNotMatched)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    insertRelease(
        R"({"version":"1.0.0"})", "signature", "1.0.0",
        "app", "linux", "arm64",
        {"/path/to/file.bin"}
    );

    EXPECT_THROW(
        service->getArchive(sc, deviceId, "app", "linux", "x64"),
        pqxx::sql_error
    );
}


TEST_F(DownloadServiceTest, GetArchiveDifferentDeviceTypeNotMatched)
{
    int64_t deviceId = insertDevice("sensor", "linux", "x64");

    insertRelease(
        R"({"version":"1.0.0"})", "signature", "1.0.0",
        "controller", "linux", "x64",
        {"/path/to/file.bin"}
    );

    EXPECT_THROW(
        service->getArchive(sc, deviceId, "sensor", "linux", "x64"),
        pqxx::sql_error
    );
}

TEST_F(DownloadServiceTest, GetArchiveInstallationTimeInUpdateInfo)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    int installationTime = 45;
    insertRelease(
        R"({"version":"1.0.0"})", "signature", "1.0.0",
        "app", "linux", "x64",
        {"/path/to/file.bin"},
        true, false, 0, installationTime
    );

    EXPECT_CALL(*mockArchivePtr, create_archive_from_paths(_, _))
        .WillOnce([](const std::vector<std::string>&,
                     std::vector<uint8_t>& archive) {
            archive = {0x01};
            return ARCHIVE_OK;
        });

    auto beforeCall = std::chrono::steady_clock::now();
    service->getArchive(sc, deviceId, "app", "linux", "x64");
    auto afterCall = std::chrono::steady_clock::now();

    auto info = getUpdateInfo(deviceId);

    auto expectedMinTimeout = beforeCall + std::chrono::minutes(installationTime + 5);
    auto expectedMaxTimeout = afterCall + std::chrono::minutes(installationTime + 5);

    EXPECT_GE(info.expireTime, expectedMinTimeout);
    EXPECT_LE(info.expireTime, expectedMaxTimeout);
}


TEST_F(DownloadServiceTest, GetArchiveMultipleCallsSameDevice)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    insertRelease(
        R"({"version":"1.0.0"})", "signature", "1.0.0",
        "app", "linux", "x64",
        {"/path/to/file.bin"}
    );

    EXPECT_CALL(*mockArchivePtr, create_archive_from_paths(_, _))
        .WillRepeatedly([](const std::vector<std::string>&,
                           std::vector<uint8_t>& archive) {
            archive = {0x01, 0x02};
            return ARCHIVE_OK;
        });

    clearUpdateInfo();
    auto result1 = service->getArchive(sc, deviceId, "app", "linux", "x64");
    EXPECT_TRUE(hasUpdateInfo(deviceId));

    clearUpdateInfo();
    auto result2 = service->getArchive(sc, deviceId, "app", "linux", "x64");
    EXPECT_TRUE(hasUpdateInfo(deviceId));

    EXPECT_EQ(result1, result2);
}


TEST_F(DownloadServiceTest, GetArchiveErrorCodeInUpdateInfo)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    insertRelease(
        R"({"version":"1.0.0"})", "signature", "1.0.0",
        "app", "linux", "x64",
        {"/path/to/file.bin"}
    );

    EXPECT_CALL(*mockArchivePtr, create_archive_from_paths(_, _))
        .WillOnce([](const std::vector<std::string>&,
                     std::vector<uint8_t>& archive) {
            archive = {0x01};
            return ARCHIVE_OK;
        });

    service->getArchive(sc, deviceId, "app", "linux", "x64");

    auto info = getUpdateInfo(deviceId);
    EXPECT_EQ(info.status, 3);
}
