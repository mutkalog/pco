#include <gtest/gtest.h>
#include <pqxx/pqxx>
#include <memory>

#include "core/servercontext.h"
#include "core/rolloutsupervisor.h"

#include "servicefixturebase.h"


class RolloutSupervisorTest : public ServiceFixtureBase
{
protected:
    std::unique_ptr<RolloutSupervisor> supervisor;

    void SetUp() override
    {
        Database::instance(testDbname, "postgres", "127.0.0.1", "5433");

        try {
            conn = std::make_unique<pqxx::connection>(connString);
        } catch (const std::exception& e) {
            FAIL() << "Could not connect to test database: " << e.what();
        }


        sc = std::make_shared<ServerContext>(nullptr, nullptr);
        supervisor = std::make_unique<RolloutSupervisor>(sc);
    }

    void TearDown() override
    {
        clearTables();
        sc->staging.rollouts.releaseToInfoMap.clear();
    }

    void callProcess() { supervisor->process(); }
    pqxx::result callCheckRollout(entry_id_t id) { return supervisor->checkRollout(*conn, id); }
    void callInvalidateRelease(entry_id_t id) { supervisor->invalidateRelease(*conn, id); }
    void callAssignDevices(entry_id_t id, const RolloutInfo& ri) { supervisor->assignDevices(*conn, {id, ri}); }
    void callUpdateCanary(entry_id_t id, const RolloutInfo& ri) { supervisor->updateCanary(*conn, {id, ri}); }
    void callCommitCanary(entry_id_t id) { supervisor->commitCanary(*conn, id); }
    void callRemoveAssignments(entry_id_t id) { supervisor->removeAssignments(*conn, id); }
    void callSetReleasesInactive(entry_id_t id, const RolloutInfo& ri) { supervisor->setReleasesInactive(*conn, {id, ri}); }

    int createRelease(bool active, bool isCanary, int canaryPercent,
                      const std::string& type, const std::string& plat, const std::string& arch,
                      const std::string& version = "1.0.0")
    {
        pqxx::work txn(*conn);
        pqxx::params params;

        params.append("dummy_manifest");
        params.append("dummy_sig");
        params.append(version);
        params.append(type);
        params.append(plat);
        params.append(arch);
        params.append("{}");
        params.append(10);
        params.append(active);
        params.append(isCanary);
        params.append(canaryPercent);

        auto res = txn.exec(
            "INSERT INTO releases ("
            "  manifest_raw, signature_raw, version, device_type, platform, arch, "
            "  file_paths, installation_time, active, is_canary, canary_percent"
            ") VALUES ("
            "  $1, $2, $3, $4, $5, $6, "
            "  $7, $8, $9, $10, $11"
            ") RETURNING id",
            params);

        txn.commit();
        return res[0][0].as<int>();
    }

    int createDevice(const std::string& type, const std::string& plat, const std::string& arch, bool old = false)
    {
        pqxx::work txn(*conn);
        pqxx::params params;
        params.append(type);
        params.append(plat);
        params.append(arch);

        std::string timeSql = old ? "NOW() - INTERVAL '1 year'" : "NOW()";

        auto res = txn.exec(
            "INSERT INTO devices (device_type, platform, arch, last_seen, poling_interval) "
            "VALUES ($1, $2, $3, " + timeSql + ", 1) RETURNING id",
            params);
        txn.commit();
        return res[0][0].as<int>();
    }

    void createAssignment(int relId, int devId, const std::string& status)
    {
        pqxx::work txn(*conn);
        pqxx::params params;
        params.append(relId);
        params.append(devId);
        params.append(status);

        txn.exec(
            "INSERT INTO release_assignments (release_id, device_id, status) VALUES ($1, $2, $3)",
            params);
        txn.commit();
    }

    bool isReleaseActive(int id)
    {
        pqxx::work txn(*conn);
        pqxx::params params;
        params.append(id);
        auto res = txn.exec("SELECT active FROM releases WHERE id = $1", params);
        return res[0][0].as<bool>();
    }

    int getCanaryPercent(int id)
    {
        pqxx::work txn(*conn);
        pqxx::params params;
        params.append(id);
        auto res = txn.exec("SELECT canary_percent FROM releases WHERE id = $1", params);
        return res[0][0].as<int>();
    }

    bool isCanary(int id)
    {
        pqxx::work txn(*conn);
        pqxx::params params;
        params.append(id);
        auto res = txn.exec("SELECT is_canary FROM releases WHERE id = $1", params);
        return res[0][0].as<bool>();
    }

    int getAssignmentCount(int relId)
    {
        pqxx::work txn(*conn);
        pqxx::params params;
        params.append(relId);
        auto res = txn.exec("SELECT count(*) FROM release_assignments WHERE release_id = $1", params);
        return res[0][0].as<int>();
    }
};


TEST_F(RolloutSupervisorTest, Method_CheckRollout_Statistics)
{
    int relId = createRelease(true, true, 10, "sensor", "linux", "x86", "2.0.0");
    int dev1 = createDevice("sensor", "linux", "x86");
    int dev2 = createDevice("sensor", "linux", "x86");
    int dev3 = createDevice("sensor", "linux", "x86");
    int dev4 = createDevice("sensor", "linux", "x86");

    createAssignment(relId, dev1, "success");
    createAssignment(relId, dev2, "success");
    createAssignment(relId, dev3, "failed");
    createAssignment(relId, dev4, "pending");

    pqxx::result res = callCheckRollout(relId);

    ASSERT_FALSE(res.empty());
    const auto& row = res[0];

    ASSERT_TRUE(row["has_failed"].as<bool>());
    ASSERT_EQ(row["success_percent"].as<int>(), 50);
    ASSERT_EQ(row["canary_percent"].as<int>(), 10);
}


TEST_F(RolloutSupervisorTest, Method_InvalidateRelease)
{
    int relId = createRelease(true, true, 50, "sensor", "linux", "x86", "2.0.1");

    callInvalidateRelease(relId);

    ASSERT_FALSE(isReleaseActive(relId));
    ASSERT_FALSE(isCanary(relId));
}


TEST_F(RolloutSupervisorTest, Method_AssignDevices)
{
    int relId = createRelease(true, true, 0, "sensor", "linux", "x86", "2.0.2");

    for (int i = 0; i < 20; ++i)
    {
        createDevice("sensor", "linux", "x86");
    }

    RolloutInfo info;
    info.type = "sensor";
    info.platform = "linux";
    info.arch = "x86";
    info.nextSelectionPercentage = 25;

    callAssignDevices(relId, info);

    int count = getAssignmentCount(relId);
    ASSERT_EQ(count, 5);
}


TEST_F(RolloutSupervisorTest, Method_UpdateCanary)
{
    int relId = createRelease(true, true, 10, "sensor", "linux", "x86", "2.0.3");

    RolloutInfo info;
    info.inRolloutPercentage = 45;

    callUpdateCanary(relId, info);

    ASSERT_EQ(getCanaryPercent(relId), 45);
}


TEST_F(RolloutSupervisorTest, Method_CommitCanary)
{
    int relId = createRelease(true, true, 90, "sensor", "linux", "x86", "2.0.4");

    callCommitCanary(relId);

    ASSERT_TRUE(isReleaseActive(relId));
    ASSERT_FALSE(isCanary(relId));
    ASSERT_EQ(getCanaryPercent(relId), 100);
}


TEST_F(RolloutSupervisorTest, Method_RemoveAssignments)
{
    int relId = createRelease(true, true, 10, "sensor", "linux", "x86", "2.0.5");
    int devId = createDevice("sensor", "linux", "x86");
    createAssignment(relId, devId, "success");

    ASSERT_EQ(getAssignmentCount(relId), 1);

    callRemoveAssignments(relId);

    ASSERT_EQ(getAssignmentCount(relId), 0);
}


TEST_F(RolloutSupervisorTest, Method_SetReleasesInactive)
{
    int oldRelId = createRelease(true, false, 100, "sensor", "linux", "x86", "1.0.0");
    int newRelId = createRelease(true, true, 10, "sensor", "linux", "x86", "2.0.0");
    int otherTypeRelId = createRelease(true, false, 100, "gateway", "linux", "arm", "1.0.0");

    RolloutInfo info;
    info.type = "sensor";
    info.platform = "linux";
    info.arch = "x86";

    callSetReleasesInactive(newRelId, info);

    ASSERT_FALSE(isReleaseActive(oldRelId));
    ASSERT_TRUE(isReleaseActive(newRelId));
    ASSERT_TRUE(isReleaseActive(otherTypeRelId));
}


TEST_F(RolloutSupervisorTest, ProcessNotCanaryRelease)
{
    int relId = createRelease(true, false, 0, "sensor", "linux", "x86", "1.0.0");

    RolloutInfo info;
    info.isCanary = false;
    info.type = "sensor";
    info.platform = "linux";
    info.arch = "x86";

    {
        std::unique_lock<std::mutex> ul(sc->staging.rollouts.mtx);
        sc->staging.rollouts.releaseToInfoMap[relId] = info;
    }

    callProcess();

    ASSERT_TRUE(sc->staging.rollouts.releaseToInfoMap.empty());
}


TEST_F(RolloutSupervisorTest, ProcessCanaryAssignDevices)
{
    int relId = createRelease(true, true, 10, "sensor", "linux", "x86", "1.1.0");

    for (int i = 0; i < 10; ++i) {
        createDevice("sensor", "linux", "x86");
    }

    RolloutInfo info;
    info.isCanary = true;
    info.inRolloutPercentage = 10;
    info.nextSelectionPercentage = 10;
    info.type = "sensor";
    info.platform = "linux";
    info.arch = "x86";

    {
        std::unique_lock<std::mutex> ul(sc->staging.rollouts.mtx);
        sc->staging.rollouts.releaseToInfoMap[relId] = info;
    }

    callProcess();

    ASSERT_GE(getAssignmentCount(relId), 1);
    ASSERT_FALSE(sc->staging.rollouts.releaseToInfoMap.empty());
}


TEST_F(RolloutSupervisorTest, ProcessCanaryFailed)
{
    int relId = createRelease(true, true, 10, "sensor", "linux", "x86", "1.2.0");
    int devId = createDevice("sensor", "linux", "x86");

    createAssignment(relId, devId, "failed");

    RolloutInfo info;
    info.isCanary = true;
    info.inRolloutPercentage = 10;
    info.type = "sensor";
    info.platform = "linux";
    info.arch = "x86";

    {
        std::unique_lock<std::mutex> ul(sc->staging.rollouts.mtx);
        sc->staging.rollouts.releaseToInfoMap[relId] = info;
    }

    callProcess();

    ASSERT_FALSE(isReleaseActive(relId));
    ASSERT_FALSE(isCanary(relId));
    ASSERT_EQ(getAssignmentCount(relId), 0);
    ASSERT_TRUE(sc->staging.rollouts.releaseToInfoMap.empty());
}


TEST_F(RolloutSupervisorTest, ProcessCanarySuccessScaleUp)
{
    int relId = createRelease(true, true, 10, "sensor", "linux", "x86", "1.3.0");
    int devId = createDevice("sensor", "linux", "x86");

    createAssignment(relId, devId, "success");

    RolloutInfo info;
    info.isCanary = true;
    info.inRolloutPercentage = 10;
    info.type = "sensor";
    info.platform = "linux";
    info.arch = "x86";

    {
        std::unique_lock<std::mutex> ul(sc->staging.rollouts.mtx);
        sc->staging.rollouts.releaseToInfoMap[relId] = info;
    }

    callProcess();

    ASSERT_EQ(getCanaryPercent(relId), 20);
    auto& updatedInfo = sc->staging.rollouts.releaseToInfoMap[relId];
    ASSERT_EQ(updatedInfo.inRolloutPercentage, 20.0);
    ASSERT_FALSE(sc->staging.rollouts.releaseToInfoMap.empty());
}


TEST_F(RolloutSupervisorTest, ProcessCanaryCommit)
{
    int relId = createRelease(true, true, 100, "sensor", "linux", "x86", "1.4.0");
    int devId = createDevice("sensor", "linux", "x86");
    createAssignment(relId, devId, "success");

    RolloutInfo info;
    info.isCanary = true;
    info.inRolloutPercentage = 100;
    info.type = "sensor";
    info.platform = "linux";
    info.arch = "x86";

    {
        std::unique_lock<std::mutex> ul(sc->staging.rollouts.mtx);
        sc->staging.rollouts.releaseToInfoMap[relId] = info;
    }

    callProcess();

    ASSERT_FALSE(isCanary(relId));
    ASSERT_TRUE(isReleaseActive(relId));
    ASSERT_EQ(getCanaryPercent(relId), 100);
    ASSERT_EQ(getAssignmentCount(relId), 0);
    ASSERT_TRUE(sc->staging.rollouts.releaseToInfoMap.empty());
}


TEST_F(RolloutSupervisorTest, ProcessCanaryInactiveDeviceFail)
{
    int relId = createRelease(true, true, 10, "sensor", "linux", "x86", "3.0.0");

    int devId = createDevice("sensor", "linux", "x86", true);

    createAssignment(relId, devId, "pending");

    RolloutInfo info;
    info.isCanary = true;
    info.inRolloutPercentage = 10;
    info.type = "sensor";
    info.platform = "linux";
    info.arch = "x86";

    {
        std::unique_lock<std::mutex> ul(sc->staging.rollouts.mtx);
        sc->staging.rollouts.releaseToInfoMap[relId] = info;
    }

    callProcess();

    ASSERT_FALSE(isReleaseActive(relId));
    ASSERT_FALSE(isCanary(relId));
    ASSERT_TRUE(sc->staging.rollouts.releaseToInfoMap.empty());
}


TEST_F(RolloutSupervisorTest, ProcessCanaryPartialSuccessNoScale)
{
    int relId = createRelease(true, true, 20, "sensor", "linux", "x86", "3.0.1");
    int dev1 = createDevice("sensor", "linux", "x86");
    int dev2 = createDevice("sensor", "linux", "x86");

    createAssignment(relId, dev1, "success");
    createAssignment(relId, dev2, "pending");

    RolloutInfo info;
    info.isCanary = true;
    info.inRolloutPercentage = 20;
    info.type = "sensor";
    info.platform = "linux";
    info.arch = "x86";

    {
        std::unique_lock<std::mutex> ul(sc->staging.rollouts.mtx);
        sc->staging.rollouts.releaseToInfoMap[relId] = info;
    }

    callProcess();

    ASSERT_EQ(getCanaryPercent(relId), 20);
    ASSERT_TRUE(isReleaseActive(relId));
    ASSERT_FALSE(sc->staging.rollouts.releaseToInfoMap.empty());
}
