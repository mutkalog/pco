#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <fstream>

#include <mocks/cryptoutils_mock.h>

#include <tests/client-test/unit/mocks/stateexecutor_mock.h>
#include <tests/client-test/unit/mocks/statepersistence_mock.h>
#include <tests/client-test/unit/mocks/deviceinfo_mock.h>
#include <tests/client-test/unit/mocks/syscalls_mock.h>
#include <tests/client-test/unit/mocks/httpclient_mock.h>
#include <tests/client-test/unit/stateexecutors/executorsfixturebase.h>

#include "core/stateexecutors/finalizingstateexecutor.h"



namespace {
const fs::path testDevConfFile            = "/tmp/devconf.json";
const fs::path testLastUpdateFile         = "/tmp/last-update.json";
const fs::path testLastUpdateRecoveryFile = testLastUpdateFile.parent_path() / "rollback" / testLastUpdateFile.filename();
const fs::path badDevConfFile             = "/fakedir/devconf.json";
const fs::path badLastUpdateFile          = "/fakedir/last-update.json";

const std::string file0Path         = "/test/myapp/app1";
const std::string file0Hash         = "c93c5405894b0fa7021fc39e32e205e40ca0abc06335fd2a024adf6a5bf28459";
const std::string file1Path         = "/test/myapp/app2";
const std::string file1Hash         = "c93c5405894b0fa7021fc39e32e205e40ca0abc06335fd2a024adf6a5bf28459";
const std::string file2Path         = "prepare.sh";
const std::string file2Hash         = "b759349b9e633738f4b8047bba51a62035e363f6e16379744a7f7840bda237aa";
const bool        file2Script       = true;
const std::string file3Path         = "commit.sh";
const std::string file3Hash         = "857256cbded0a1789906bb0ac93a3ac32bd7ff90746e6964d997a9468938d5ef";
const bool        file3Script       = true;
const std::string releaseArch       = "ARMv8";
const std::string releasePlatform   = "Linux";
const std::string releaseType       = "Raspberry Pi 4";
const std::string releaseVersion    = "1.2.1";
const std::string releaseVersionNew = "1.3.0";
const std::string releaseTimestamp  = "2025-11-12T10:23:00Z";
const std::string stagingDir        = "/tmp/pco/";


enum : uint32_t {START_STATE, USUAL_STATE};

auto createManifest = [](const std::string& version) {
    ArtifactManifest m;
    ArtifactManifest::File f0; f0.isScript = false; f0.installPath = file0Path; m.files.push_back(f0);
    ArtifactManifest::File f1; f1.isScript = false; f1.installPath = file1Path; m.files.push_back(f1);
    ArtifactManifest::File f2; f2.isScript = true;  f2.installPath = file2Path; m.files.push_back(f2);
    ArtifactManifest::File f3; f3.isScript = true;  f3.installPath = file3Path; m.files.push_back(f3);
    m.release = {version, releaseType, releasePlatform, releaseArch, tm{}};
    return m;
};

const ArtifactManifest oldManifest = createManifest(releaseVersion);
const ArtifactManifest newManifest = createManifest(releaseVersionNew);

} // namespace


using namespace testing;


class FinalizingStateExecutorAllPublic : public FinalizingStateExecutor
{
public:
    using FinalizingStateExecutor::rollback;
    using FinalizingStateExecutor::launchScript;
    using FinalizingStateExecutor::totalCleanup;

    // void sleep(std::chrono::minutes m) override {};
    FinalizingStateExecutorAllPublic() : FinalizingStateExecutor(FINALIZING) {}
};


class FinalizingStateExecutorSpy : public FinalizingStateExecutorAllPublic
{
public:
    MOCK_METHOD(void, rollback, (UpdateContext &ctx), (override));
    MOCK_METHOD(void, launchScript, (UpdateContext &ctx, const fs::path &scriptPath), (override));
    MOCK_METHOD(void, totalCleanup, (UpdateContext &ctx), (override));
};


class FinalizingStateExecutorTestFixture : public ExecutorsFixtureBase
{
protected:
    fs::path testRoot = fs::temp_directory_path() / "test";

    void SetUp() override
    {
        auto mockDevConf          = std::make_unique<NiceMock<MockClientConfig>>();
        auto mockHttpClient       = std::make_unique<NiceMock<MockHttpClient>>();
        auto mockStatePersistence = std::make_unique<NiceMock<MockStatePersistence>>();
        auto mockCryptoUtils      = std::make_unique<NiceMock<MockCryptoUtils>>();
        auto mockSyscalls         = std::make_unique<NiceMock<MockSystemCalls>>();

        targetSe = std::make_unique<NiceMock<MockStateExecutor>>(); targetSep = targetSe.get();
        failSe   = std::make_unique<NiceMock<MockStateExecutor>>(); failSep   = failSe.get();

        std::unordered_map<uint32_t, std::unique_ptr<IStateExecutor>> idToStateMap;
        idToStateMap.emplace(START_STATE, std::make_unique<NiceMock<MockStateExecutor>>());
        idToStateMap.emplace(StateExecutor::FINALIZING, std::make_unique<FinalizingStateExecutorAllPublic>());
        idToStateMap.emplace(StateExecutor::IDLE, std::move(targetSe));

        sm = std::make_unique<StateMachineTestAllPublic>(
            std::move(mockStatePersistence),
            UpdateContext(std::move(mockDevConf), std::move(mockHttpClient), std::move(mockCryptoUtils), nullptr, std::move(mockSyscalls), stagingDir, ""),
            std::move(idToStateMap),
            START_STATE,
            StateExecutor::VERIFYING
        );
    }

    void TearDown() override
    {
        fs::remove_all(testRoot);
    }
};


TEST_F(FinalizingStateExecutorTestFixture, RollbackSuccessRestoresFilesAndClearsState)
{
    auto it = sm->idToStateMap_.find(StateExecutor::FINALIZING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto finalizingExecutor = dynamic_cast<FinalizingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(finalizingExecutor, nullptr);

    fs::path app1        = testRoot / "app1";
    fs::path app2        = testRoot / "app2";
    fs::path rbApp1      = app1.parent_path() / "rollback" / app1.filename();
    fs::path rbApp2      = app2.parent_path() / "rollback" / app2.filename();

    fs::create_directories(rbApp1.parent_path());
    fs::create_directories(rbApp2.parent_path());

    {
        std::ofstream(rbApp1) << "rollback1";
        std::ofstream(rbApp2) << "rollback2";
    }

    ASSERT_FALSE(fs::exists(app1));
    ASSERT_FALSE(fs::exists(app2));
    ASSERT_TRUE(fs::exists(rbApp1));
    ASSERT_TRUE(fs::exists(rbApp2));

    auto& ctx = sm->context;
    ctx.recovering = false;
    ctx.busyResources.rollbacks = 1;

    ctx.pathToRollbackPathMap.insert({app1, rbApp1});
    ctx.pathToRollbackPathMap.insert({app2, rbApp2});

    ASSERT_NO_THROW({
        finalizingExecutor->rollback(ctx);
    });

    ASSERT_TRUE(fs::exists(app1));
    ASSERT_TRUE(fs::exists(app2));
    ASSERT_FALSE(fs::exists(rbApp1));
    ASSERT_FALSE(fs::exists(rbApp2));

    ASSERT_TRUE(ctx.pathToRollbackPathMap.empty());
    ASSERT_EQ(ctx.busyResources.rollbacks, 0);
}

TEST_F(FinalizingStateExecutorTestFixture, RollbackAlreadyCommitted)
{
    auto it = sm->idToStateMap_.find(StateExecutor::FINALIZING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto finalizingExecutor = dynamic_cast<FinalizingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(finalizingExecutor, nullptr);

    fs::path app1        = testRoot / "app1";
    fs::path app2        = testRoot / "app2";
    fs::path rbApp1      = app1.parent_path() / "rollback" / app1.filename();
    fs::path rbApp2      = app2.parent_path() / "rollback" / app2.filename();

    fs::create_directories(rbApp1.parent_path());
    fs::create_directories(rbApp2.parent_path());

    {
        std::ofstream(app1)   << "rollback1";
        std::ofstream(rbApp2) << "rollback2";
    }

    ASSERT_TRUE(fs::exists(app1));
    ASSERT_FALSE(fs::exists(app2));
    ASSERT_FALSE(fs::exists(rbApp1));
    ASSERT_TRUE(fs::exists(rbApp2));

    auto& ctx = sm->context;
    ctx.recovering = true;
    ctx.busyResources.rollbacks = 1;

    ctx.pathToRollbackPathMap.insert({app1, rbApp1});
    ctx.pathToRollbackPathMap.insert({app2, rbApp2});

    ASSERT_NO_THROW({
        finalizingExecutor->rollback(ctx);
    });

    ASSERT_TRUE(fs::exists(app1));
    ASSERT_TRUE(fs::exists(app2));
    ASSERT_FALSE(fs::exists(rbApp1));
    ASSERT_FALSE(fs::exists(rbApp2));

    ASSERT_TRUE(ctx.pathToRollbackPathMap.empty());
    ASSERT_EQ(ctx.busyResources.rollbacks, 0);
}


TEST_F(FinalizingStateExecutorTestFixture, RollbackFirstUpdate)
{
    auto it = sm->idToStateMap_.find(StateExecutor::FINALIZING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto finalizingExecutor = dynamic_cast<FinalizingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(finalizingExecutor, nullptr);

    auto& ctx = sm->context;
    ctx.recovering = false;
    ctx.busyResources.rollbacks = 0;

    ctx.pathToRollbackPathMap.clear();

    ASSERT_NO_THROW({
        finalizingExecutor->rollback(ctx);
    });

    ASSERT_TRUE(ctx.pathToRollbackPathMap.empty());
    ASSERT_EQ(ctx.busyResources.rollbacks, 0);
}


TEST_F(FinalizingStateExecutorTestFixture, TotalCleanupSuccessRemovesEverythingAndResetsContext)
{
    auto it = sm->idToStateMap_.find(StateExecutor::FINALIZING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto finalizingExecutor = dynamic_cast<FinalizingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(finalizingExecutor, nullptr);

    auto& ctx = sm->context;

    ctx.stagingDir = testRoot / "staging";
    fs::create_directories(ctx.stagingDir / "sub");
    {
        std::ofstream(ctx.stagingDir / "file.txt") << "data";
    }

    ASSERT_TRUE(fs::exists(ctx.stagingDir));
    ctx.busyResources.stagingDirCreated = 1;

    fs::path app1   = testRoot / "app1";
    fs::path rbApp1 = app1.parent_path() / "rollback" / app1.filename();

    fs::create_directories(rbApp1.parent_path());
    {
        std::ofstream(rbApp1) << "rollback";
    }

    ASSERT_TRUE(fs::exists(rbApp1));

    ctx.busyResources.rollbacks = 1;
    ctx.pathToRollbackPathMap.insert({app1, rbApp1});

    ASSERT_EQ(setenv("PCO_NEW_ARTIFACTS_PATHS", "dummy", 1), 0);
    ASSERT_EQ(setenv("PCO_STAGING_ARTIFACTS_PATHS", "dummy", 1), 0);

    ctx.recovering = true;
    ctx.rollback   = true;

    ASSERT_NO_THROW({
        finalizingExecutor->totalCleanup(ctx);
    });

    ASSERT_FALSE(fs::exists(ctx.stagingDir));
    ASSERT_EQ(ctx.busyResources.stagingDirCreated, 0);

    ASSERT_FALSE(fs::exists(rbApp1));
    ASSERT_TRUE(ctx.pathToRollbackPathMap.empty());
    ASSERT_EQ(ctx.busyResources.rollbacks, 0);

    ASSERT_EQ(getenv("PCO_NEW_ARTIFACTS_PATHS"), nullptr);
    ASSERT_EQ(getenv("PCO_STAGING_ARTIFACTS_PATHS"), nullptr);

    ASSERT_FALSE(ctx.recovering);
    ASSERT_FALSE(ctx.rollback);

    uint32_t word = 0;
    std::memcpy(&word, &ctx.busyResources, sizeof(BusyResources));
    ASSERT_EQ(word, 0u);
}


TEST_F(FinalizingStateExecutorTestFixture, LaunchScriptSuccess)
{
    auto it = sm->idToStateMap_.find(StateExecutor::FINALIZING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto finalizingExecutor =
        dynamic_cast<FinalizingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(finalizingExecutor, nullptr);

    auto& ctx = sm->context;

    fs::path script = testRoot / "rollback.sh";
    fs::create_directories(testRoot);
    {
        std::ofstream(script) << "#!/bin/sh\nexit 0\n";
    }

    auto mockSys = static_cast<MockSystemCalls*>(sm->context.syscalls.get());
    EXPECT_CALL(*mockSys, chmod(_, 0755))
        .WillOnce(testing::Return(0));

    EXPECT_CALL(*mockSys, posix_spawn(_, _, _, _, _, _))
        .WillOnce([](pid_t* pid, const char*, const void*, const void*, char* const[], char* const[]) {
        *pid = 1234;
        return 0;
    });

    EXPECT_CALL(*mockSys, waitpid(1234, _, 0))
        .WillOnce([](pid_t, int* status, int){
        *status = 0;
        return 1234;
    });

    ASSERT_NO_THROW({
        finalizingExecutor->launchScript(ctx, script);
    });
}


class FinalizingStateExecutorTestSpyFixture : public FinalizingStateExecutorTestFixture
{
protected:
    fs::path testRoot = fs::temp_directory_path() / "test";

    void SetUp() override
    {
        auto mockDevConf          = std::make_unique<NiceMock<MockClientConfig>>();
        auto mockHttpClient       = std::make_unique<NiceMock<MockHttpClient>>();
        auto mockStatePersistence = std::make_unique<NiceMock<MockStatePersistence>>();
        auto mockCryptoUtils      = std::make_unique<NiceMock<MockCryptoUtils>>();
        auto mockSyscalls         = std::make_unique<NiceMock<MockSystemCalls>>();

        targetSe = std::make_unique<NiceMock<MockStateExecutor>>(); targetSep = targetSe.get();
        failSe   = std::make_unique<NiceMock<MockStateExecutor>>(); failSep   = failSe.get();

        std::unordered_map<uint32_t, std::unique_ptr<IStateExecutor>> idToStateMap;
        idToStateMap.emplace(START_STATE, std::make_unique<NiceMock<MockStateExecutor>>());
        idToStateMap.emplace(StateExecutor::FINALIZING, std::make_unique<FinalizingStateExecutorSpy>());
        idToStateMap.emplace(StateExecutor::IDLE, std::move(targetSe));

        sm = std::make_unique<StateMachineTestAllPublic>(
            std::move(mockStatePersistence),
            UpdateContext(std::move(mockDevConf), std::move(mockHttpClient), std::move(mockCryptoUtils), nullptr, std::move(mockSyscalls), stagingDir, ""),
            std::move(idToStateMap),
            START_STATE,
            StateExecutor::VERIFYING
        );
    }

    void TearDown() override
    {
        fs::remove_all(testRoot);
    }
};


TEST_F(FinalizingStateExecutorTestSpyFixture, ExecuteSuccessUpdateTransitionsToIdle)
{
    EXPECT_CALL(*targetSep, id())
        .WillOnce(testing::Return(StateExecutor::IDLE));

    auto it = sm->idToStateMap_.find(StateExecutor::FINALIZING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto finalizingExecutor = dynamic_cast<FinalizingStateExecutorSpy*>(it->second.get());
    ASSERT_NE(finalizingExecutor, nullptr);

    auto& ctx = sm->context;

    ctx.rollback = false;
    ctx.manifest = newManifest;
    ctx.reportMessage = {0, ""};

    auto devConfMock = static_cast<MockClientConfig*>(ctx.devconf.get());
    EXPECT_CALL(*devConfMock, saveNewUpdateInfo)
        .Times(1);

    EXPECT_CALL(*devConfMock, type())
        .WillRepeatedly(Return(releaseType));
    EXPECT_CALL(*devConfMock, arch())
        .WillRepeatedly(Return(releaseArch));
    EXPECT_CALL(*devConfMock, platform())
        .WillRepeatedly(Return(releasePlatform));
    EXPECT_CALL(*devConfMock, id())
        .WillRepeatedly(Return(33));

    EXPECT_CALL(*finalizingExecutor, rollback)
        .Times(0);

    EXPECT_CALL(*finalizingExecutor, totalCleanup)
        .Times(1);

    auto response = std::make_unique<httplib::Response>();
    response->status = httplib::OK_200;
    httplib::Result result(std::move(response), httplib::Error::Success);

    auto httpMock = static_cast<MockHttpClient*>(sm->context.client.get());
    EXPECT_CALL(*httpMock, Post("/report", _, "application/json"))
        .WillOnce(testing::Return(std::move(result)));

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, clear)
        .Times(1);

    finalizingExecutor->execute(*sm);

    ASSERT_EQ(sm->state(), StateExecutor::IDLE);
    ASSERT_FALSE(ctx.rollback);
    ASSERT_TRUE(ctx.manifest.files.empty());
}


TEST_F(FinalizingStateExecutorTestSpyFixture, ExecuteRollbackTransitToIdle)
{
    EXPECT_CALL(*targetSep, id())
        .WillOnce(testing::Return(StateExecutor::IDLE));

    auto it = sm->idToStateMap_.find(StateExecutor::FINALIZING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto finalizingExecutor = dynamic_cast<FinalizingStateExecutorSpy*>(it->second.get());
    ASSERT_NE(finalizingExecutor, nullptr);

    auto& ctx = sm->context;

    ctx.rollback = true;
    ctx.manifest = newManifest;
    ctx.reportMessage = {INTERNAL_UPDATE_ERROR, "FAILED"};

    auto devConfMock = static_cast<MockClientConfig*>(ctx.devconf.get());
    EXPECT_CALL(*devConfMock, saveNewUpdateInfo)
        .Times(0);

    EXPECT_CALL(*devConfMock, type())
        .WillRepeatedly(Return(releaseType));
    EXPECT_CALL(*devConfMock, arch())
        .WillRepeatedly(Return(releaseArch));
    EXPECT_CALL(*devConfMock, platform())
        .WillRepeatedly(Return(releasePlatform));
    EXPECT_CALL(*devConfMock, id())
        .WillRepeatedly(Return(33));

    EXPECT_CALL(*finalizingExecutor, rollback)
        .WillOnce([&](UpdateContext& ctx) {
            ctx.rollback = false;
    });

    EXPECT_CALL(*finalizingExecutor, totalCleanup)
        .Times(1);

    auto response = std::make_unique<httplib::Response>();
    response->status = httplib::OK_200;
    httplib::Result result(std::move(response), httplib::Error::Success);

    auto httpMock = static_cast<MockHttpClient*>(sm->context.client.get());
    EXPECT_CALL(*httpMock, Post("/report", _, "application/json"))
        .WillOnce(testing::Return(std::move(result)));

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, clear)
        .Times(1);

    finalizingExecutor->execute(*sm);

    ASSERT_EQ(sm->state(), StateExecutor::IDLE);
    ASSERT_FALSE(ctx.rollback);
    ASSERT_TRUE(ctx.manifest.files.empty());
}


TEST_F(FinalizingStateExecutorTestSpyFixture, ExecuteCleanupFailRebootDeathTest)
{
    auto it = sm->idToStateMap_.find(StateExecutor::FINALIZING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto finalizingExecutor = dynamic_cast<FinalizingStateExecutorSpy*>(it->second.get());
    ASSERT_NE(finalizingExecutor, nullptr);

    auto& ctx = sm->context;
    ctx.rollback = true;
    ctx.manifest = newManifest;
    ctx.reportMessage = {INTERNAL_UPDATE_ERROR, "FAILED"};

    auto devConfMock = static_cast<MockClientConfig*>(ctx.devconf.get());
    auto httpMock = static_cast<MockHttpClient*>(sm->context.client.get());
    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());

    auto response = std::make_unique<httplib::Response>();
    response->status = httplib::OK_200;
    httplib::Result result(std::move(response), httplib::Error::Success);

    ASSERT_EXIT(
    {
        EXPECT_CALL(*targetSep, id())
            .WillOnce(::testing::Return(StateExecutor::IDLE));

        EXPECT_CALL(*devConfMock, type())
            .WillRepeatedly(Return(releaseType));
        EXPECT_CALL(*devConfMock, arch())
            .WillRepeatedly(Return(releaseArch));
        EXPECT_CALL(*devConfMock, platform())
            .WillRepeatedly(Return(releasePlatform));
        EXPECT_CALL(*devConfMock, id())
            .WillRepeatedly(Return(33));

        EXPECT_CALL(*finalizingExecutor, rollback).Times(1);
        EXPECT_CALL(*finalizingExecutor, totalCleanup)
            .WillOnce(testing::Throw(std::system_error()));

        EXPECT_CALL(*httpMock, Post("/report", _, "application/json"))
            .WillOnce(testing::Return(std::move(result)));

        EXPECT_CALL(*spMock, clear).Times(1);

        finalizingExecutor->execute(*sm);
    },
    ::testing::ExitedWithCode(EXIT_FAILURE),
    ".*"
    );
}


TEST_F(FinalizingStateExecutorTestSpyFixture, ExecuteRollbackHttpPostReturnsNullptr)
{
    EXPECT_CALL(*targetSep, id())
        .WillOnce(testing::Return(StateExecutor::IDLE));

    auto it = sm->idToStateMap_.find(StateExecutor::FINALIZING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto finalizingExecutor = dynamic_cast<FinalizingStateExecutorSpy*>(it->second.get());
    ASSERT_NE(finalizingExecutor, nullptr);

    auto& ctx = sm->context;

    ctx.rollback = false;
    ctx.manifest = newManifest;
    ctx.reportMessage = { INTERNAL_UPDATE_ERROR, "FAILED" };

    auto devConfMock = static_cast<MockClientConfig*>(ctx.devconf.get());
    EXPECT_CALL(*devConfMock, saveNewUpdateInfo)
        .Times(1);

    EXPECT_CALL(*devConfMock, type())
        .WillRepeatedly(Return(releaseType));
    EXPECT_CALL(*devConfMock, arch())
        .WillRepeatedly(Return(releaseArch));
    EXPECT_CALL(*devConfMock, platform())
        .WillRepeatedly(Return(releasePlatform));
    EXPECT_CALL(*devConfMock, id())
        .WillRepeatedly(Return(33));

    EXPECT_CALL(*finalizingExecutor, rollback)
        .Times(0);
    EXPECT_CALL(*finalizingExecutor, totalCleanup)
        .Times(1);

    auto httpMock = static_cast<MockHttpClient*>(sm->context.client.get());
    EXPECT_CALL(*httpMock, Post("/report", _, "application/json"))
    .Times(4)
    .WillRepeatedly([]() {
        return httplib::Result(nullptr, httplib::Error::Connection);
    });

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, clear)
        .Times(1);

    finalizingExecutor->execute(*sm);

    ASSERT_EQ(sm->state(), StateExecutor::IDLE);
    ASSERT_FALSE(ctx.rollback);
    ASSERT_TRUE(ctx.manifest.files.empty());
}


TEST_F(FinalizingStateExecutorTestFixture, RollbackFailsWhenRenameThrows)
{
    auto it = sm->idToStateMap_.find(StateExecutor::FINALIZING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto finalizingExecutor = dynamic_cast<FinalizingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(finalizingExecutor, nullptr);

    fs::path app1   = testRoot / "app1";
    fs::path rbApp1 = testRoot / "nonexistent_rollback" / "app1";

    auto& ctx = sm->context;
    ctx.recovering = false;
    ctx.busyResources.rollbacks = 1;
    ctx.pathToRollbackPathMap.insert({app1, rbApp1});

    ASSERT_THROW(finalizingExecutor->rollback(ctx), std::system_error);
}


TEST_F(FinalizingStateExecutorTestFixture, RollbackWithScriptLaunchesRollbackScript)
{
    auto it = sm->idToStateMap_.find(StateExecutor::FINALIZING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto finalizingExecutor = dynamic_cast<FinalizingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(finalizingExecutor, nullptr);

    auto& ctx = sm->context;
    ctx.recovering = false;
    ctx.busyResources.rollbacks = 0;
    ctx.pathToRollbackPathMap.clear();
    ctx.stagingDir = testRoot / "staging";

    fs::create_directories(ctx.stagingDir);
    fs::path scriptPath = ctx.stagingDir / "rollback.sh";
    {
        std::ofstream(scriptPath) << "#!/bin/sh\nexit 0\n";
    }

    ArtifactManifest manifest;
    ArtifactManifest::File rollbackScript;
    rollbackScript.isScript = true;
    rollbackScript.installPath = "rollback.sh";
    manifest.files.push_back(rollbackScript);
    ctx.manifest = manifest;

    auto mockSys = static_cast<MockSystemCalls*>(ctx.syscalls.get());
    EXPECT_CALL(*mockSys, chmod(_, 0755))
        .WillOnce(Return(0));

    EXPECT_CALL(*mockSys, posix_spawn(_, _, _, _, _, _))
        .WillOnce([](pid_t* pid, const char*, const void*, const void*, char* const[], char* const[]) {
            *pid = 5678;
            return 0;
        });

    EXPECT_CALL(*mockSys, waitpid(5678, _, 0))
        .WillOnce([](pid_t, int* status, int) {
            *status = 0;
            return 5678;
        });

    ASSERT_NO_THROW(finalizingExecutor->rollback(ctx));
}


TEST_F(FinalizingStateExecutorTestFixture, LaunchScriptFailsOnChmod)
{
    auto it = sm->idToStateMap_.find(StateExecutor::FINALIZING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto finalizingExecutor = dynamic_cast<FinalizingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(finalizingExecutor, nullptr);

    auto& ctx = sm->context;
    fs::path script = testRoot / "rollback.sh";
    fs::create_directories(testRoot);
    {
        std::ofstream(script) << "#!/bin/sh\nexit 0\n";
    }

    auto mockSys = static_cast<MockSystemCalls*>(ctx.syscalls.get());
    EXPECT_CALL(*mockSys, chmod(_, 0755))
        .WillOnce(Return(-1));

    ASSERT_THROW(finalizingExecutor->launchScript(ctx, script), std::runtime_error);
}


TEST_F(FinalizingStateExecutorTestFixture, LaunchScriptFailsOnPosixSpawn)
{
    auto it = sm->idToStateMap_.find(StateExecutor::FINALIZING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto finalizingExecutor = dynamic_cast<FinalizingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(finalizingExecutor, nullptr);

    auto& ctx = sm->context;
    fs::path script = testRoot / "rollback.sh";
    fs::create_directories(testRoot);
    {
        std::ofstream(script) << "#!/bin/sh\nexit 0\n";
    }

    auto mockSys = static_cast<MockSystemCalls*>(ctx.syscalls.get());
    EXPECT_CALL(*mockSys, chmod(_, 0755))
        .WillOnce(Return(0));

    EXPECT_CALL(*mockSys, posix_spawn(_, _, _, _, _, _))
        .WillOnce(Return(ENOENT));

    ASSERT_THROW(finalizingExecutor->launchScript(ctx, script), std::runtime_error);
}


TEST_F(FinalizingStateExecutorTestFixture, LaunchScriptFailsOnWaitpid)
{
    auto it = sm->idToStateMap_.find(StateExecutor::FINALIZING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto finalizingExecutor = dynamic_cast<FinalizingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(finalizingExecutor, nullptr);

    auto& ctx = sm->context;
    fs::path script = testRoot / "rollback.sh";
    fs::create_directories(testRoot);
    {
        std::ofstream(script) << "#!/bin/sh\nexit 0\n";
    }

    auto mockSys = static_cast<MockSystemCalls*>(ctx.syscalls.get());
    EXPECT_CALL(*mockSys, chmod(_, 0755))
        .WillOnce(Return(0));

    EXPECT_CALL(*mockSys, posix_spawn(_, _, _, _, _, _))
        .WillOnce([](pid_t* pid, const char*, const void*, const void*, char* const[], char* const[]) {
            *pid = 9999;
            return 0;
        });

    EXPECT_CALL(*mockSys, waitpid(9999, _, 0))
        .WillOnce(Return(-1));

    ASSERT_THROW(finalizingExecutor->launchScript(ctx, script), std::runtime_error);
}



TEST_F(FinalizingStateExecutorTestFixture, TotalCleanupOnlyStagingDir)
{
    auto it = sm->idToStateMap_.find(StateExecutor::FINALIZING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto finalizingExecutor = dynamic_cast<FinalizingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(finalizingExecutor, nullptr);

    auto& ctx = sm->context;

    ctx.stagingDir = testRoot / "staging";
    fs::create_directories(ctx.stagingDir);
    {
        std::ofstream(ctx.stagingDir / "file.txt") << "data";
    }
    ctx.busyResources.stagingDirCreated = 1;
    ctx.busyResources.rollbacks = 0;
    ctx.pathToRollbackPathMap.clear();

    ASSERT_EQ(setenv("PCO_NEW_ARTIFACTS_PATHS", "dummy", 1), 0);
    ASSERT_EQ(setenv("PCO_STAGING_ARTIFACTS_PATHS", "dummy", 1), 0);

    ctx.recovering = true;
    ctx.rollback = true;

    ASSERT_NO_THROW(finalizingExecutor->totalCleanup(ctx));

    ASSERT_FALSE(fs::exists(ctx.stagingDir));
    ASSERT_EQ(ctx.busyResources.stagingDirCreated, 0);
    ASSERT_EQ(ctx.busyResources.rollbacks, 0);
    ASSERT_FALSE(ctx.recovering);
    ASSERT_FALSE(ctx.rollback);
}


TEST_F(FinalizingStateExecutorTestFixture, TotalCleanupOnlyRollbacks)
{
    auto it = sm->idToStateMap_.find(StateExecutor::FINALIZING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto finalizingExecutor = dynamic_cast<FinalizingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(finalizingExecutor, nullptr);

    auto& ctx = sm->context;

    ctx.stagingDir = testRoot / "staging";
    ctx.busyResources.stagingDirCreated = 0;

    fs::path app1   = testRoot / "app1";
    fs::path rbApp1 = app1.parent_path() / "rollback" / app1.filename();

    fs::create_directories(rbApp1.parent_path());
    {
        std::ofstream(rbApp1) << "rollback";
    }

    ctx.busyResources.rollbacks = 1;
    ctx.pathToRollbackPathMap.insert({app1, rbApp1});

    ASSERT_EQ(setenv("PCO_NEW_ARTIFACTS_PATHS", "dummy", 1), 0);
    ASSERT_EQ(setenv("PCO_STAGING_ARTIFACTS_PATHS", "dummy", 1), 0);

    ctx.recovering = false;
    ctx.rollback = false;

    ASSERT_NO_THROW(finalizingExecutor->totalCleanup(ctx));

    ASSERT_FALSE(fs::exists(rbApp1));
    ASSERT_TRUE(ctx.pathToRollbackPathMap.empty());
    ASSERT_EQ(ctx.busyResources.rollbacks, 0);
}


TEST_F(FinalizingStateExecutorTestFixture, TotalCleanupNothingToClean)
{
    auto it = sm->idToStateMap_.find(StateExecutor::FINALIZING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto finalizingExecutor = dynamic_cast<FinalizingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(finalizingExecutor, nullptr);

    auto& ctx = sm->context;

    ctx.stagingDir = testRoot / "staging";
    ctx.busyResources.stagingDirCreated = 0;
    ctx.busyResources.rollbacks = 0;
    ctx.pathToRollbackPathMap.clear();

    ASSERT_EQ(setenv("PCO_NEW_ARTIFACTS_PATHS", "dummy", 1), 0);
    ASSERT_EQ(setenv("PCO_STAGING_ARTIFACTS_PATHS", "dummy", 1), 0);

    ctx.recovering = true;
    ctx.rollback = true;

    ASSERT_NO_THROW(finalizingExecutor->totalCleanup(ctx));

    ASSERT_FALSE(ctx.recovering);
    ASSERT_FALSE(ctx.rollback);
}


// ==================== Дополнительные тесты для FinalizingStateExecutorTestSpyFixture ====================

TEST_F(FinalizingStateExecutorTestSpyFixture, ExecuteSaveNewUpdateInfoThrowsStillTransitionsToIdle)
{
    EXPECT_CALL(*targetSep, id())
        .WillOnce(Return(StateExecutor::IDLE));

    auto it = sm->idToStateMap_.find(StateExecutor::FINALIZING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto finalizingExecutor = dynamic_cast<FinalizingStateExecutorSpy*>(it->second.get());
    ASSERT_NE(finalizingExecutor, nullptr);

    auto& ctx = sm->context;
    ctx.rollback = false;
    ctx.manifest = newManifest;
    ctx.reportMessage = {0, ""};

    auto devConfMock = static_cast<MockClientConfig*>(ctx.devconf.get());
    EXPECT_CALL(*devConfMock, saveNewUpdateInfo)
        .WillOnce(Throw(std::runtime_error("cannot save")));

    EXPECT_CALL(*devConfMock, type()).WillRepeatedly(Return(releaseType));
    EXPECT_CALL(*devConfMock, arch()).WillRepeatedly(Return(releaseArch));
    EXPECT_CALL(*devConfMock, platform()).WillRepeatedly(Return(releasePlatform));
    EXPECT_CALL(*devConfMock, id()).WillRepeatedly(Return(33));

    EXPECT_CALL(*finalizingExecutor, rollback).Times(0);
    EXPECT_CALL(*finalizingExecutor, totalCleanup).Times(1);

    auto response = std::make_unique<httplib::Response>();
    response->status = httplib::OK_200;
    httplib::Result result(std::move(response), httplib::Error::Success);

    auto httpMock = static_cast<MockHttpClient*>(ctx.client.get());
    EXPECT_CALL(*httpMock, Post("/report", _, "application/json"))
        .WillOnce(Return(std::move(result)));

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, clear).Times(1);

    finalizingExecutor->execute(*sm);

    ASSERT_EQ(sm->state(), StateExecutor::IDLE);
    EXPECT_EQ(ctx.reportMessage.first, INTERNAL_UPDATE_ERROR);
}


TEST_F(FinalizingStateExecutorTestSpyFixture, ExecuteRollbackThrowsStillTransitionsToIdle)
{
    EXPECT_CALL(*targetSep, id())
        .WillOnce(Return(StateExecutor::IDLE));

    auto it = sm->idToStateMap_.find(StateExecutor::FINALIZING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto finalizingExecutor = dynamic_cast<FinalizingStateExecutorSpy*>(it->second.get());
    ASSERT_NE(finalizingExecutor, nullptr);

    auto& ctx = sm->context;
    ctx.rollback = true;
    ctx.manifest = newManifest;
    ctx.reportMessage = {0, ""};

    auto devConfMock = static_cast<MockClientConfig*>(ctx.devconf.get());
    EXPECT_CALL(*devConfMock, saveNewUpdateInfo).Times(0);
    EXPECT_CALL(*devConfMock, type()).WillRepeatedly(Return(releaseType));
    EXPECT_CALL(*devConfMock, arch()).WillRepeatedly(Return(releaseArch));
    EXPECT_CALL(*devConfMock, platform()).WillRepeatedly(Return(releasePlatform));
    EXPECT_CALL(*devConfMock, id()).WillRepeatedly(Return(33));

    EXPECT_CALL(*finalizingExecutor, rollback)
        .WillOnce(Throw(std::system_error(std::make_error_code(std::errc::io_error))));

    EXPECT_CALL(*finalizingExecutor, totalCleanup).Times(1);

    auto response = std::make_unique<httplib::Response>();
    response->status = httplib::OK_200;
    httplib::Result result(std::move(response), httplib::Error::Success);

    auto httpMock = static_cast<MockHttpClient*>(ctx.client.get());
    EXPECT_CALL(*httpMock, Post("/report", _, "application/json"))
        .WillOnce(Return(std::move(result)));

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, clear).Times(1);

    finalizingExecutor->execute(*sm);

    ASSERT_EQ(sm->state(), StateExecutor::IDLE);
    EXPECT_EQ(ctx.reportMessage.first, INTERNAL_UPDATE_ERROR);
}


TEST_F(FinalizingStateExecutorTestSpyFixture, ExecuteReportContainsCorrectData)
{
    EXPECT_CALL(*targetSep, id())
        .WillOnce(Return(StateExecutor::IDLE));

    auto it = sm->idToStateMap_.find(StateExecutor::FINALIZING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto finalizingExecutor = dynamic_cast<FinalizingStateExecutorSpy*>(it->second.get());
    ASSERT_NE(finalizingExecutor, nullptr);

    auto& ctx = sm->context;
    ctx.rollback = false;
    ctx.manifest = newManifest;
    ctx.reportMessage = {42, "custom error message"};

    auto devConfMock = static_cast<MockClientConfig*>(ctx.devconf.get());
    EXPECT_CALL(*devConfMock, saveNewUpdateInfo).Times(1);
    EXPECT_CALL(*devConfMock, type()).WillRepeatedly(Return("TestType"));
    EXPECT_CALL(*devConfMock, arch()).WillRepeatedly(Return("TestArch"));
    EXPECT_CALL(*devConfMock, platform()).WillRepeatedly(Return("TestPlatform"));
    EXPECT_CALL(*devConfMock, id()).WillRepeatedly(Return(12345));

    EXPECT_CALL(*finalizingExecutor, rollback).Times(0);
    EXPECT_CALL(*finalizingExecutor, totalCleanup).Times(1);

    json capturedReport;
    auto httpMock = static_cast<MockHttpClient*>(ctx.client.get());
    EXPECT_CALL(*httpMock, Post("/report", _, "application/json"))
        .WillOnce([&capturedReport](const std::string&, const std::string& body, const std::string&) {
            capturedReport = json::parse(body);
            auto response = std::make_unique<httplib::Response>();
            response->status = httplib::OK_200;
            return httplib::Result(std::move(response), httplib::Error::Success);
        });

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, clear).Times(1);

    finalizingExecutor->execute(*sm);

    ASSERT_EQ(sm->state(), StateExecutor::IDLE);

    EXPECT_EQ(capturedReport["type"], "TestType");
    EXPECT_EQ(capturedReport["arch"], "TestArch");
    EXPECT_EQ(capturedReport["platform"], "TestPlatform");
    EXPECT_EQ(capturedReport["id"], 12345);
    EXPECT_EQ(capturedReport["status"], "SUCCESS");
    EXPECT_EQ(capturedReport["current_version"], releaseVersionNew);
    EXPECT_EQ(capturedReport["error"]["code"], 42);
    EXPECT_EQ(capturedReport["error"]["message"], "custom error message");
}


TEST_F(FinalizingStateExecutorTestSpyFixture, ExecuteHttpRetrySucceedsOnThirdAttempt)
{
    EXPECT_CALL(*targetSep, id())
        .WillOnce(Return(StateExecutor::IDLE));

    auto it = sm->idToStateMap_.find(StateExecutor::FINALIZING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto finalizingExecutor = dynamic_cast<FinalizingStateExecutorSpy*>(it->second.get());
    ASSERT_NE(finalizingExecutor, nullptr);

    auto& ctx = sm->context;
    ctx.rollback = false;
    ctx.manifest = newManifest;

    auto devConfMock = static_cast<MockClientConfig*>(ctx.devconf.get());
    EXPECT_CALL(*devConfMock, saveNewUpdateInfo).Times(1);
    EXPECT_CALL(*devConfMock, type()).WillRepeatedly(Return(releaseType));
    EXPECT_CALL(*devConfMock, arch()).WillRepeatedly(Return(releaseArch));
    EXPECT_CALL(*devConfMock, platform()).WillRepeatedly(Return(releasePlatform));
    EXPECT_CALL(*devConfMock, id()).WillRepeatedly(Return(33));

    EXPECT_CALL(*finalizingExecutor, rollback).Times(0);
    EXPECT_CALL(*finalizingExecutor, totalCleanup).Times(1);

    auto httpMock = static_cast<MockHttpClient*>(ctx.client.get());
    EXPECT_CALL(*httpMock, Post("/report", _, "application/json"))
        .WillOnce([]() {
            return httplib::Result(nullptr, httplib::Error::Connection);
        })
        .WillOnce([]() {
            return httplib::Result(nullptr, httplib::Error::Connection);
        })
        .WillOnce([]() {
            auto response = std::make_unique<httplib::Response>();
            response->status = httplib::OK_200;
            return httplib::Result(std::move(response), httplib::Error::Success);
        });

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, clear).Times(1);

    finalizingExecutor->execute(*sm);

    ASSERT_EQ(sm->state(), StateExecutor::IDLE);
}

