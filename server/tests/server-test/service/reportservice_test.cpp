#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>
#include <chrono>

#include "core/service/reportservice.h"
#include "core/database.h"

#include "servicefixturebase.h"

using json = nlohmann::ordered_json;
using ::testing::NiceMock;

class ReportServiceTest : public ServiceFixtureBase
{
protected:
    std::unique_ptr<ReportService> service;

    void SetUp() override
    {
        Database::instance(testDbname, "postgres", "127.0.0.1", "5433");

        try {
            conn = std::make_unique<pqxx::connection>(connString);
        } catch (const std::exception& e) {
            FAIL() << "Could not connect to test database: " << e.what();
        }

        clearTables();
        sc = std::make_shared<ServerContext>(nullptr, nullptr);
        service = std::make_unique<ReportService>();
    }

    void TearDown() override
    {
        clearUpdateInfo();
        clearTables();
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

    void addUpdateInfo(uint64_t devId, uint64_t relId, bool finished = false,
                       uint64_t errCode = 0, const std::string& report = "")
    {
        auto& updates = sc->staging.updates;
        std::lock_guard<std::mutex> lg(updates.mtx);

        UpdateInfo info {
            relId,
            finished,
            errCode,
            report,
            std::chrono::steady_clock::now() + std::chrono::minutes(30)
        };

        updates.devToReleaseMap.insert({devId, info});
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

    json createValidReport(uint64_t deviceId, int errorCode)
    {
        json report;
        report["id"] = deviceId;
        report["error"]["code"] = errorCode;
        return report;
    }

    json createFullReport(uint64_t deviceId, int errorCode,
                          const std::string& message = "Success")
    {
        json report;
        report["id"] = deviceId;
        report["error"]["code"] = errorCode;
        report["error"]["message"] = message;
        report["timestamp"] = "2024-01-15T10:30:00Z";
        return report;
    }
};


TEST_F(ReportServiceTest, ParseReportDeviceNotExistsThrows)
{
    uint64_t nonExistentDeviceId = 99999;
    json report = createValidReport(nonExistentDeviceId, 0);

    ASSERT_THROW(
        service->parseReport(sc, report),
        std::runtime_error
    );
}


TEST_F(ReportServiceTest, ParseReportValidReportUpdatesInfo)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");
    uint64_t releaseId = 42;

    addUpdateInfo(deviceId, releaseId);

    json report = createValidReport(deviceId, 0);

    ASSERT_NO_THROW(service->parseReport(sc, report));

    auto info = getUpdateInfo(deviceId);
    ASSERT_TRUE(info.finished);
    ASSERT_EQ(info.status, 0);
}


TEST_F(ReportServiceTest, ParseReportUpdatesFinishedFlag)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    addUpdateInfo(deviceId, 1, false);

    auto infoBefore = getUpdateInfo(deviceId);
    ASSERT_FALSE(infoBefore.finished);

    json report = createValidReport(deviceId, 0);
    service->parseReport(sc, report);

    auto infoAfter = getUpdateInfo(deviceId);
    ASSERT_TRUE(infoAfter.finished);
}


TEST_F(ReportServiceTest, ParseReportUpdatesStatusSuccess)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    addUpdateInfo(deviceId, 1, false, 999);

    json report = createValidReport(deviceId, 0);
    service->parseReport(sc, report);

    auto info = getUpdateInfo(deviceId);
    ASSERT_EQ(info.status, 0);
}


TEST_F(ReportServiceTest, ParseReportUpdatesReportDump)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    addUpdateInfo(deviceId, 1, false, 0, "");

    json report = createFullReport(deviceId, 0, "SUCCESS");
    service->parseReport(sc, report);

    auto info = getUpdateInfo(deviceId);
    ASSERT_FALSE(info.report.empty());

    json parsedReport = json::parse(info.report);
    ASSERT_EQ(parsedReport["id"].get<uint64_t>(), static_cast<uint64_t>(deviceId));
    ASSERT_EQ(parsedReport["error"]["code"].get<int>(), 0);
    ASSERT_EQ(parsedReport["error"]["message"].get<std::string>(), "SUCCESS");
}


TEST_F(ReportServiceTest, ParseReportMissingIdFieldThrows)
{
    json report;
    report["error"]["code"] = 0;

    ASSERT_THROW(
        service->parseReport(sc, report),
        json::exception
    );
}


TEST_F(ReportServiceTest, ParseReportMissingErrorFieldThrows)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    json report;
    report["id"] = deviceId;

    ASSERT_THROW(
        service->parseReport(sc, report),
        json::exception
    );
}


TEST_F(ReportServiceTest, ParseReportMultipleDevicesIndependent)
{
    int64_t deviceId1 = insertDevice("app", "linux", "x64");
    int64_t deviceId2 = insertDevice("sensor", "windows", "arm64");

    addUpdateInfo(deviceId1, 10);
    addUpdateInfo(deviceId2, 20);

    json report1 = createValidReport(deviceId1, 0);
    json report2 = createValidReport(deviceId2, 3);

    service->parseReport(sc, report1);

    auto info1 = getUpdateInfo(deviceId1);
    auto info2 = getUpdateInfo(deviceId2);

    ASSERT_TRUE(info1.finished);
    ASSERT_FALSE(info2.finished);

    service->parseReport(sc, report2);

    info2 = getUpdateInfo(deviceId2);
    ASSERT_TRUE(info2.finished);
    ASSERT_EQ(info2.status, 3);
}


TEST_F(ReportServiceTest, ParseReportDeviceExistsButDifferentIdInDb)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    uint64_t wrongDeviceId = deviceId + 1000;

    json report = createValidReport(wrongDeviceId, 0);

    ASSERT_THROW(
        service->parseReport(sc, report),
        std::runtime_error
    );
}


TEST_F(ReportServiceTest, ParseReportSequentialReportsOverwrite)
{
    int64_t deviceId = insertDevice("app", "linux", "x64");

    addUpdateInfo(deviceId, 1);

    json report1 = createValidReport(deviceId, 1);
    service->parseReport(sc, report1);

    auto info1 = getUpdateInfo(deviceId);
    ASSERT_EQ(info1.status, 1);

    json report2 = createValidReport(deviceId, 2);
    service->parseReport(sc, report2);

    auto info2 = getUpdateInfo(deviceId);
    ASSERT_EQ(info2.status, 2);
}
