#ifndef MANIFESTSERVICETEST_H
#define MANIFESTSERVICETEST_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>
#include <thread>

#include "core/service/manifestservice.h"
#include "core/database.h"

#include "servicefixturebase.h"


using json = nlohmann::ordered_json;

class ManifestServiceTest : public ServiceFixtureBase
{
protected:
    std::unique_ptr<ManifestService> service;

    void SetUp() override
    {
        Database::instance(testDbname, "postgres", "127.0.0.1", "5433");

        try {
            conn = std::make_unique<pqxx::connection>(connString);
        } catch (const std::exception& e) {
            FAIL() << "Could not connect to test database: " << e.what();
        }

        clearTables();

        service = std::make_unique<ManifestService>();
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
                          bool active = true,
                          bool isCanary = false,
                          int canaryPercent = 0,
                          int installationTime = 60)
    {
        pqxx::work txn(*conn);
        auto result = txn.exec_params(
            "INSERT INTO releases "
            "(manifest_raw, signature_raw, version, device_type, platform, arch, "
            "file_paths, active, is_canary, canary_percent, installation_time) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11) "
            "RETURNING id",
            manifestRaw, signatureRaw, version, deviceType, platform, arch,
            "{}", active, isCanary, canaryPercent, installationTime
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
        auto result = txn.exec_params(
            "INSERT INTO devices (device_type, platform, arch, poling_interval) "
            "VALUES ($1, $2, $3, $4) RETURNING id",
            deviceType, platform, arch, pollingInterval
        );
        txn.commit();
        return result[0][0].as<int64_t>();
    }

    void insertReleaseAssignment(int64_t releaseId, int64_t deviceId,
                                 const std::string& status = "pending")
    {
        pqxx::work txn(*conn);
        txn.exec_params(
            "INSERT INTO release_assignments (release_id, device_id, status) "
            "VALUES ($1, $2, $3)",
            releaseId, deviceId, status
        );
        txn.commit();
    }

    std::optional<std::string> getDeviceLastSeen(int64_t deviceId)
    {
        pqxx::work txn(*conn);
        auto result = txn.exec_params(
            "SELECT last_seen FROM devices WHERE id = $1",
            deviceId
        );
        txn.commit();

        if (result.empty() || result[0][0].is_null())
        {
            return std::nullopt;
        }
        return result[0][0].as<std::string>();
    }
};



TEST_F(ManifestServiceTest, GetManifestNoDeviceThrows)
{
    uint64_t nonExistentDeviceId = 99999;

    ASSERT_THROW(
        service->getManifest(nonExistentDeviceId, "app", "linux", "x64"),
        std::runtime_error
    );
}


TEST_F(ManifestServiceTest, GetManifestNoReleaseThrows)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    ASSERT_THROW(
        service->getManifest(deviceId, "app", "linux", "x64"),
        std::runtime_error
    );
}


TEST_F(ManifestServiceTest, GetManifestNoMatchingReleaseThrows)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    insertRelease(
        R"({"version":"1.0.0"})", "sig123", "1.0.0",
        "app", "windows", "x64"
    );

    ASSERT_THROW(
        service->getManifest(deviceId, "app", "linux", "x64"),
        std::runtime_error
    );
}


TEST_F(ManifestServiceTest, GetManifestFallbackReleaseReturnsCorrect)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    std::string expectedManifest = R"({"version":"2.0.0","type":"app"})";
    std::string expectedSignature = "valid_signature_abc123";

    insertRelease(
        expectedManifest, expectedSignature, "2.0.0",
        "app", "linux", "x64", true, false
    );

    json result = service->getManifest(deviceId, "app", "linux", "x64");

    ASSERT_EQ(result["manifest"].get<std::string>(), expectedManifest);
    ASSERT_EQ(result["signature"].get<std::string>(), expectedSignature);
}


TEST_F(ManifestServiceTest, GetManifestCanaryReleaseReturnsCorrect)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    std::string canaryManifest = R"({"version":"3.0.0-canary"})";
    std::string canarySignature = "canary_signature_xyz";

    int64_t canaryReleaseId = insertRelease(
        canaryManifest, canarySignature, "3.0.0-canary",
        "app", "linux", "x64", true, true, 10
    );

    insertReleaseAssignment(canaryReleaseId, deviceId);

    json result = service->getManifest(deviceId, "app", "linux", "x64");

    ASSERT_EQ(result["manifest"].get<std::string>(), canaryManifest);
    ASSERT_EQ(result["signature"].get<std::string>(), canarySignature);
}


TEST_F(ManifestServiceTest, GetManifestCanaryPriorityOverFallback)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    std::string fallbackManifest = R"({"version":"1.0.0"})";
    insertRelease(
        fallbackManifest, "fallback_sig", "1.0.0",
        "app", "linux", "x64", true, false
    );

    std::string canaryManifest = R"({"version":"2.0.0-canary"})";
    std::string canarySignature = "canary_sig";
    int64_t canaryReleaseId = insertRelease(
        canaryManifest, canarySignature, "2.0.0-canary",
        "app", "linux", "x64", true, true, 10
    );

    insertReleaseAssignment(canaryReleaseId, deviceId);

    json result = service->getManifest(deviceId, "app", "linux", "x64");

    ASSERT_EQ(result["manifest"].get<std::string>(), canaryManifest);
    ASSERT_EQ(result["signature"].get<std::string>(), canarySignature);
}


TEST_F(ManifestServiceTest, GetManifestUpdatesLastSeen)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    insertRelease(
        R"({"version":"1.0.0"})", "signature", "1.0.0",
        "app", "linux", "x64"
    );

    auto lastSeenBefore = getDeviceLastSeen(deviceId);
    ASSERT_FALSE(lastSeenBefore.has_value());

    service->getManifest(deviceId, "app", "linux", "x64");

    auto lastSeenAfter = getDeviceLastSeen(deviceId);
    ASSERT_TRUE(lastSeenAfter.has_value());
}


TEST_F(ManifestServiceTest, GetManifestInactiveReleaseNotReturned)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    insertRelease(
        R"({"version":"1.0.0"})", "signature", "1.0.0",
        "app", "linux", "x64", false, false
    );

    ASSERT_THROW(
        service->getManifest(deviceId, "app", "linux", "x64"),
        std::runtime_error
    );
}


TEST_F(ManifestServiceTest, GetManifestCanaryWithoutAssignmentNotReturned)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    insertRelease(
        R"({"version":"2.0.0-canary"})", "canary_sig", "2.0.0-canary",
        "app", "linux", "x64", true, true, 10
    );

    ASSERT_THROW(
        service->getManifest(deviceId, "app", "linux", "x64"),
        std::runtime_error
    );
}


TEST_F(ManifestServiceTest, GetManifestReturnsJsonWithCorrectStructure)
{
    int64_t deviceId = insertDevice("sensor", "windows", "arm64");

    insertRelease(
        R"({"key":"value"})", "test_signature", "1.0.0",
        "sensor", "windows", "arm64"
    );

    json result = service->getManifest(deviceId, "sensor", "windows", "arm64");

    ASSERT_TRUE(result.contains("manifest"));
    ASSERT_TRUE(result.contains("signature"));
    ASSERT_EQ(result.size(), 2);
}


TEST_F(ManifestServiceTest, GetManifestDifferentArchNotMatched)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    insertRelease(
        R"({"version":"1.0.0"})", "signature", "1.0.0",
        "app", "linux", "arm64"
    );

    ASSERT_THROW(
        service->getManifest(deviceId, "app", "linux", "x64"),
        std::runtime_error
    );
}


TEST_F(ManifestServiceTest, GetManifestDifferentDeviceTypeNotMatched)
{
    int64_t deviceId = insertDevice("sensor", "linux", "x64");

    insertRelease(
        R"({"version":"1.0.0"})", "signature", "1.0.0",
        "controller", "linux", "x64"
    );

    ASSERT_THROW(
        service->getManifest(deviceId, "sensor", "linux", "x64"),
        std::runtime_error
    );
}


TEST_F(ManifestServiceTest, GetManifestEmptyManifestRawReturnsEmpty)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    insertRelease("", "signature", "1.0.0", "app", "linux", "x64");

    json result = service->getManifest(deviceId, "app", "linux", "x64");

    ASSERT_EQ(result["manifest"].get<std::string>(), "");
    ASSERT_EQ(result["signature"].get<std::string>(), "signature");
}


TEST_F(ManifestServiceTest, GetManifestMultipleCallsUpdateLastSeenEachTime)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    insertRelease(
        R"({"version":"1.0.0"})", "signature", "1.0.0",
        "app", "linux", "x64"
    );

    service->getManifest(deviceId, "app", "linux", "x64");
    auto firstLastSeen = getDeviceLastSeen(deviceId);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    service->getManifest(deviceId, "app", "linux", "x64");
    auto secondLastSeen = getDeviceLastSeen(deviceId);

    ASSERT_TRUE(firstLastSeen.has_value());
    ASSERT_TRUE(secondLastSeen.has_value());
}

#endif // MANIFESTSERVICETEST_H
