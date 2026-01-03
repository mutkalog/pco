#include <gtest/gtest.h>
#include <pqxx/pqxx>
#include <chrono>

#include "core/servercontext.h"
#include "core/updatesupervisor.h"

#include "servicefixturebase.h"


class UpdateSupervisorTest : public ServiceFixtureBase
{
protected:
    std::unique_ptr<UpdateSupervisor> supervisor;

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
        supervisor = std::make_unique<UpdateSupervisor>(sc);
    }

    void TearDown() override
    {
        supervisor.reset();
        clearTables();
        sc->staging.updates.devToReleaseMap.clear();
    }

    void callProcessUpdate(uint64_t devId, const UpdateInfo& info) { supervisor->processUpdate({devId, info}); }
    void callProcess() { supervisor->process(); }

    int createRelease(const std::string& version = "1.0.0")
    {
        pqxx::work txn(*conn);
        pqxx::params params;
        params.append("dummy_manifest");
        params.append("dummy_sig");
        params.append(version);
        params.append("sensor");
        params.append("linux");
        params.append("x86");
        params.append("{}");
        params.append(10);
        params.append(true);
        params.append(false);
        params.append(0);

        auto res = txn.exec(
            "INSERT INTO releases (manifest_raw, signature_raw, version, device_type, platform, arch, "
            "file_paths, installation_time, active, is_canary, canary_percent) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11) RETURNING id",
            params);
        txn.commit();
        return res[0][0].as<int>();
    }

    int createDevice(int currentReleaseId = 0)
    {
        pqxx::work txn(*conn);
        pqxx::params params;
        params.append("sensor");
        params.append("linux");
        params.append("x86");

        auto res = txn.exec(
            "INSERT INTO devices (device_type, platform, arch, last_seen, poling_interval) "
            "VALUES ($1, $2, $3, NOW(), 1) RETURNING id",
            params);

        int devId = res[0][0].as<int>();

        if (currentReleaseId != 0) {
            txn.exec0(
                "UPDATE devices SET release_id = " +
                std::to_string(currentReleaseId) +
                " WHERE id = " +
                std::to_string(devId));
        }

        txn.commit();
        return devId;
    }

    void createAssignment(int relId, int devId, const std::string& status)
    {
        pqxx::work txn(*conn);
        pqxx::params params;
        params.append(relId);
        params.append(devId);
        params.append(status);

        txn.exec(
            "INSERT INTO release_assignments (release_id, device_id, status) "
            "VALUES ($1, $2, $3)",
            params);

        txn.commit();
    }

    std::string getAssignmentStatus(int relId, int devId)
    {
        pqxx::work txn(*conn);
        pqxx::params params;
        params.append(relId);
        params.append(devId);

        auto res = txn.exec(
            "SELECT status FROM release_assignments "
            "WHERE release_id = $1 AND device_id = $2",
            params);

        return res.empty() ? "" : res[0][0].as<std::string>();
    }

    int getDeviceReleaseId(int devId)
    {
        pqxx::work txn(*conn);
        pqxx::params params;
        params.append(devId);

        auto res = txn.exec(
            "SELECT release_id FROM devices WHERE id = $1",
            params);

        return (res.empty() || res[0][0].is_null())
            ? 0
            : res[0][0].as<int>();
    }

    std::pair<int, std::string> getReport(int devId, int relId)
    {
        pqxx::work txn(*conn);
        pqxx::params params;
        params.append(devId);
        params.append(relId);

        auto res = txn.exec(
            "SELECT status, body FROM reports "
            "WHERE device_id = $1 AND release_id = $2",
            params);

        return res.empty()
            ? std::make_pair(-1, std::string(""))
            : std::make_pair(
                  res[0]["status"].as<int>(),
                  res[0]["body"].as<std::string>());
    }
};


TEST_F(UpdateSupervisorTest, ProcessUpdateSuccess)
{
    int oldRelId = createRelease("1.0");
    int newRelId = createRelease("2.0");
    int devId = createDevice(oldRelId);
    createAssignment(newRelId, devId, "pending");

    UpdateInfo info;
    info.releaseId = newRelId;
    info.finished = true;
    info.status = 0;
    info.report = R"({"status": "ok"})";

    callProcessUpdate(devId, info);

    ASSERT_EQ(getAssignmentStatus(newRelId, devId), "success");
    ASSERT_EQ(getDeviceReleaseId(devId), newRelId);

    auto [status, body] = getReport(devId, newRelId);
    ASSERT_EQ(status, 0);
    ASSERT_EQ(body, info.report);
}


TEST_F(UpdateSupervisorTest, ProcessUpdateFailure)
{
    int oldRelId = createRelease("1.0");
    int newRelId = createRelease("2.0");
    int devId = createDevice(oldRelId);
    createAssignment(newRelId, devId, "pending");

    UpdateInfo info;
    info.releaseId = newRelId;
    info.finished = true;
    info.status = 3;
    info.report = R"({"error": "installation failed"})";

    callProcessUpdate(devId, info);

    ASSERT_EQ(getAssignmentStatus(newRelId, devId), "failed");
    ASSERT_EQ(getDeviceReleaseId(devId), oldRelId);

    auto [status, body] = getReport(devId, newRelId);
    ASSERT_EQ(status, 3);
    ASSERT_EQ(body, info.report);
}


TEST_F(UpdateSupervisorTest, ProcessUpdateTimeout)
{
    int oldRelId = createRelease("1.0");
    int newRelId = createRelease("2.0");
    int devId = createDevice(oldRelId);
    createAssignment(newRelId, devId, "pending");

    UpdateInfo info;
    info.releaseId = newRelId;
    info.finished = false;
    info.status = 0;

    callProcessUpdate(devId, info);

    ASSERT_EQ(getAssignmentStatus(newRelId, devId), "failed");
    ASSERT_EQ(getDeviceReleaseId(devId), oldRelId);

    auto [status, body] = getReport(devId, newRelId);
    ASSERT_EQ(status, 3);
    ASSERT_NE(body.find("timeout"), std::string::npos);
}


TEST_F(UpdateSupervisorTest, ProcessNotExpiredWait)
{
    int newRelId = createRelease("2.0.0");
    int devId = createDevice();

    UpdateInfo info;
    info.releaseId = newRelId;
    info.finished = false;
    info.expireTime = std::chrono::steady_clock::now() + std::chrono::hours(1);

    {
        std::unique_lock<std::mutex> ul(sc->staging.updates.mtx);
        sc->staging.updates.devToReleaseMap[devId] = info;
    }

    callProcess();

    ASSERT_FALSE(sc->staging.updates.devToReleaseMap.empty());
    ASSERT_EQ(getAssignmentStatus(newRelId, devId), "");
}


TEST_F(UpdateSupervisorTest, ProcessBatchMixedResults)
{
    int relId = createRelease("3.0.0");
    int devSuccess = createDevice();
    int devFail = createDevice();
    int devTimeout = createDevice();

    createAssignment(relId, devSuccess, "pending");
    createAssignment(relId, devFail, "pending");
    createAssignment(relId, devTimeout, "pending");

    UpdateInfo infoSuccess;
    infoSuccess.releaseId = relId;
    infoSuccess.finished = true;
    infoSuccess.status = 0;
    infoSuccess.report = "{}";
    infoSuccess.expireTime = std::chrono::steady_clock::now() + std::chrono::hours(1);

    UpdateInfo infoFail;
    infoFail.releaseId = relId;
    infoFail.finished = true;
    infoFail.status = 500;
    infoFail.report = "{\"err\":\"disk full\"}";
    infoFail.expireTime = std::chrono::steady_clock::now() + std::chrono::hours(1);

    UpdateInfo infoTimeout;
    infoTimeout.releaseId = relId;
    infoTimeout.finished = false;
    infoTimeout.expireTime = std::chrono::steady_clock::now() - std::chrono::seconds(1);

    {
        std::unique_lock<std::mutex> ul(sc->staging.updates.mtx);
        sc->staging.updates.devToReleaseMap[devSuccess] = infoSuccess;
        sc->staging.updates.devToReleaseMap[devFail] = infoFail;
        sc->staging.updates.devToReleaseMap[devTimeout] = infoTimeout;
    }

    callProcess();

    ASSERT_EQ(getAssignmentStatus(relId, devSuccess), "success");
    ASSERT_EQ(getDeviceReleaseId(devSuccess), relId);

    ASSERT_EQ(getAssignmentStatus(relId, devFail), "failed");
    ASSERT_EQ(getDeviceReleaseId(devFail), 0);

    ASSERT_EQ(getAssignmentStatus(relId, devTimeout), "failed");
    ASSERT_EQ(getDeviceReleaseId(devTimeout), 0);

    auto [statusT, bodyT] = getReport(devTimeout, relId);
    ASSERT_EQ(statusT, 3);
    ASSERT_NE(bodyT.find("timeout"), std::string::npos);

    ASSERT_TRUE(sc->staging.updates.devToReleaseMap.empty());
}


TEST_F(UpdateSupervisorTest, ProcessImmediateFinishFutureExpiry)
{
    int relId = createRelease("4.0.0");
    int devId = createDevice();
    createAssignment(relId, devId, "pending");

    UpdateInfo info;
    info.releaseId = relId;
    info.finished = true;
    info.status = 0;
    info.report = R"({"result":"ok"})";
    info.expireTime = std::chrono::steady_clock::now() + std::chrono::hours(999);

    {
        std::unique_lock<std::mutex> ul(sc->staging.updates.mtx);
        sc->staging.updates.devToReleaseMap[devId] = info;
    }

    callProcess();

    ASSERT_EQ(getAssignmentStatus(relId, devId), "success");
    ASSERT_TRUE(sc->staging.updates.devToReleaseMap.empty());
}
