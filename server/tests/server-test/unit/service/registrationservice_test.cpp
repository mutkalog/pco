#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "core/service/registrationservice.h"
#include "servicefixturebase.h"
#include "core/database.h"

using json = nlohmann::ordered_json;

class RegistrationServiceTest : public ServiceFixtureBase
{
protected:
    std::unique_ptr<RegistrationService> service;

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

        service = std::make_unique<RegistrationService>();
    }

    void TearDown() override
    {
        clearTables();
    }

    json createValidBody(const std::string& type = "app",
                         const std::string& platform = "linux",
                         const std::string& arch = "x64",
                         int pollingInterval = 300)
    {
        json body;
        body["type"] = type;
        body["platform"] = platform;
        body["arch"] = arch;
        body["pollingInterval"] = pollingInterval;
        return body;
    }

    std::optional<std::tuple<std::string, std::string, std::string, int>>
    getDeviceFromDb(uint64_t deviceId)
    {
        pqxx::work txn(*conn);
        pqxx::params params;
        params.append(deviceId);

        auto result = txn.exec(
            "SELECT device_type, platform, arch, poling_interval "
            "FROM devices WHERE id = $1",
            params
        );
        txn.commit();

        if (result.empty())
        {
            return std::nullopt;
        }

        return std::make_tuple(
            result[0]["device_type"].as<std::string>(),
            result[0]["platform"].as<std::string>(),
            result[0]["arch"].as<std::string>(),
            result[0]["poling_interval"].as<int>()
        );
    }
};


TEST_F(RegistrationServiceTest, RegisterDeviceSuccess)
{
    json body = createValidBody("sensor", "windows", "arm64", 600);

    uint64_t id = service->registerDevice(body);

    ASSERT_GT(id, 0u);

    auto deviceOpt = getDeviceFromDb(id);
    ASSERT_TRUE(deviceOpt.has_value());

    auto [type, platform, arch, interval] = deviceOpt.value();
    ASSERT_EQ(type, "sensor");
    ASSERT_EQ(platform, "windows");
    ASSERT_EQ(arch, "arm64");
    ASSERT_EQ(interval, 600);
}


TEST_F(RegistrationServiceTest, RegisterDeviceReturnsUniqueIds)
{
    json body1 = createValidBody("app", "linux", "x64");
    json body2 = createValidBody("sensor", "windows", "arm64");

    uint64_t id1 = service->registerDevice(body1);
    uint64_t id2 = service->registerDevice(body2);

    ASSERT_NE(id1, id2);
    ASSERT_GT(id2, id1);
}


TEST_F(RegistrationServiceTest, RegisterDeviceMissingTypeThrows)
{
    json body;
    body["platform"] = "linux";
    body["arch"] = "x64";
    body["pollingInterval"] = 300;

    ASSERT_THROW(service->registerDevice(body), json::exception);
}


TEST_F(RegistrationServiceTest, RegisterDeviceMissingPlatformThrows)
{
    json body;
    body["type"] = "app";
    body["arch"] = "x64";
    body["pollingInterval"] = 300;

    ASSERT_THROW(service->registerDevice(body), json::exception);
}


TEST_F(RegistrationServiceTest, RegisterDeviceMissingArchThrows)
{
    json body;
    body["type"] = "app";
    body["platform"] = "linux";
    body["pollingInterval"] = 300;

    ASSERT_THROW(service->registerDevice(body), json::exception);
}


TEST_F(RegistrationServiceTest, RegisterDeviceMissingPollingIntervalThrows)
{
    json body;
    body["type"] = "app";
    body["platform"] = "linux";
    body["arch"] = "x64";

    ASSERT_THROW(service->registerDevice(body), json::exception);
}


TEST_F(RegistrationServiceTest, RegisterDeviceEmptyTypeThrows)
{
    json body = createValidBody("", "linux", "x64");

    ASSERT_THROW(service->registerDevice(body), std::runtime_error);
}


TEST_F(RegistrationServiceTest, RegisterDeviceEmptyPlatformThrows)
{
    json body = createValidBody("app", "", "x64");

    ASSERT_THROW(service->registerDevice(body), std::runtime_error);
}


TEST_F(RegistrationServiceTest, RegisterDeviceEmptyArchThrows)
{
    json body = createValidBody("app", "linux", "");

    ASSERT_THROW(service->registerDevice(body), std::runtime_error);
}


TEST_F(RegistrationServiceTest, RegisterDeviceZeroPollingInterval)
{
    json body = createValidBody("app", "linux", "x64", 0);

    uint64_t id = service->registerDevice(body);

    ASSERT_GT(id, 0u);

    auto deviceOpt = getDeviceFromDb(id);
    ASSERT_TRUE(deviceOpt.has_value());

    auto [type, platform, arch, interval] = deviceOpt.value();
    ASSERT_EQ(interval, 0);
}

