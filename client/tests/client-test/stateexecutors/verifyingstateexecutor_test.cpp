#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <mocks/cryptoutils_mock.h>
#include <tests/client-test/mocks/stateexecutor_mock.h>
#include <tests/client-test/mocks/statepersistence_mock.h>
#include <tests/client-test/mocks/deviceinfo_mock.h>
#include <tests/client-test/mocks/httpclient_mock.h>
#include <tests/client-test/stateexecutors/executorsfixturebase.h>

#include "core/stateexecutors/verifyingstateexecutor.h"



namespace {
const fs::path testDevConfFile            = "/tmp/devconf.json";
const fs::path testLastUpdateFile         = "/tmp/last-update.json";
const fs::path testLastUpdateRecoveryFile = testLastUpdateFile.parent_path() / "rollback" / testLastUpdateFile.filename();
const fs::path badDevConfFile             = "/fakedir/devconf.json";
const fs::path badLastUpdateFile          = "/fakedir/last-update.json";

const std::string file0Path         = "/opt/myapp/app1";
const std::string file0Hash         = "c93c5405894b0fa7021fc39e32e205e40ca0abc06335fd2a024adf6a5bf28459";
const std::string file1Path         = "/opt/myapp/app2";
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

const std::string pcoStagingArtifactPaths = stagingDir + "app1:" + stagingDir + "app2";

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


class VerifyingStateExecutorTest : public VerifyingStateExecutor
{
public:
    using VerifyingStateExecutor::verifyHashes;
    using VerifyingStateExecutor::setEnvVar;
    using VerifyingStateExecutor::hash_t;

    VerifyingStateExecutorTest() : VerifyingStateExecutor(VERIFYING) {}
};


class VerifyingStateExecutorSpy : public VerifyingStateExecutorTest
{
public:
    MOCK_METHOD(void, verifyHashes, (const hash_t& lhs, const hash_t& rhs), (const, override));
    MOCK_METHOD(void, setEnvVar, (const std::string& key, const std::string& value), (const, override));
};


TEST(VerifyingStateExecutorTest, VerifyHashesEqual)
{
    VerifyingStateExecutorTest exec;
    VerifyingStateExecutorTest::hash_t a{1,2,3,4};
    VerifyingStateExecutorTest::hash_t b{1,2,3,4};

    ASSERT_NO_THROW(exec.verifyHashes(a, b));
}


TEST(VerifyingStateExecutorTest, VerifyHashesSizeMismatch)
{
    VerifyingStateExecutorTest exec;
    VerifyingStateExecutorTest::hash_t a{1,2,3};
    VerifyingStateExecutorTest::hash_t b{1,2,3,4};

    ASSERT_THROW(exec.verifyHashes(a, b), std::runtime_error);
}


TEST(VerifyingStateExecutorTest, VerifyHashesValueMismatch)
{
    VerifyingStateExecutorTest exec;
    VerifyingStateExecutorTest::hash_t a{1,2,3,4};
    VerifyingStateExecutorTest::hash_t b{1,2,0,4};

    ASSERT_THROW(exec.verifyHashes(a, b), std::runtime_error);
}


TEST(VerifyingStateExecutorTest, SetEnvVarSuccess)
{
    VerifyingStateExecutorTest exec;
    std::string key = "TEST_ENV_VAR";
    std::string value = "12345";

    ASSERT_NO_THROW(exec.setEnvVar(key, value));

    const char* envValue = ::getenv(key.c_str());
    ASSERT_NE(envValue, nullptr);
    ASSERT_STREQ(envValue, value.c_str());

    EXPECT_EQ(::unsetenv(key.c_str()), 0);
}


TEST(VerifyingStateExecutorTest, SetEnvVarInvalidKeyThrows)
{
    VerifyingStateExecutorTest exec;

    EXPECT_THROW(exec.setEnvVar("", "value"), std::system_error);
}


class VerifyingStateExecutorTestFixture : public ExecutorsFixtureBase
{
protected:
    void SetUp() override
    {
        auto mockDevConf          = std::make_unique<NiceMock<MockClientConfig>>();
        auto mockStatePersistence = std::make_unique<NiceMock<MockStatePersistence>>();
        auto mockCryptoUtils      = std::make_unique<NiceMock<MockCryptoUtils>>();

        targetSe = std::make_unique<NiceMock<MockStateExecutor>>(); targetSep = targetSe.get();
        failSe   = std::make_unique<NiceMock<MockStateExecutor>>(); failSep   = failSe.get();

        std::unordered_map<uint32_t, std::unique_ptr<IStateExecutor>> idToStateMap;
        idToStateMap.emplace(START_STATE, std::make_unique<NiceMock<MockStateExecutor>>());
        idToStateMap.emplace(StateExecutor::VERIFYING, std::make_unique<VerifyingStateExecutorSpy>());
        idToStateMap.emplace(StateExecutor::PREPARING, std::move(targetSe));
        idToStateMap.emplace(StateExecutor::FINALIZING, std::move(failSe));

        sm = std::make_unique<StateMachineTestAllPublic>(
            std::move(mockStatePersistence),
            UpdateContext(std::move(mockDevConf), nullptr, std::move(mockCryptoUtils), nullptr, nullptr, stagingDir, ""),
            std::move(idToStateMap),
            START_STATE,
            StateExecutor::VERIFYING
        );
    }
};


TEST_F(VerifyingStateExecutorTestFixture, ExecuteSuccessTransitionsToPreparing)
{
    EXPECT_CALL(*targetSep, id())
        .WillOnce(testing::Return(StateExecutor::PREPARING));

    sm->context.manifest = newManifest;

    auto it = sm->idToStateMap_.find(StateExecutor::VERIFYING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto verifyingExecutor = dynamic_cast<VerifyingStateExecutorSpy*>(it->second.get());
    ASSERT_NE(verifyingExecutor, nullptr);

    EXPECT_CALL(*verifyingExecutor, verifyHashes(_, _))
        .Times(sm->context.manifest.files.size());

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, dump)
        .Times(1);

    auto cryptoMock = static_cast<MockCryptoUtils*>(sm->context.cryptoUtils.get());
    EXPECT_CALL(*cryptoMock, sha256FromFile)
        .Times(sm->context.manifest.files.size())
        .WillRepeatedly(Return(std::vector<uint8_t>{1,2,3,1}));

    EXPECT_CALL(*verifyingExecutor, setEnvVar("PCO_STAGING_ARTIFACTS_PATHS", pcoStagingArtifactPaths))
        .Times(1);

    verifyingExecutor->execute(*sm);

    ASSERT_EQ(sm->state(), StateExecutor::PREPARING);
    ASSERT_FALSE(sm->context.rollback);
}


TEST_F(VerifyingStateExecutorTestFixture, ExecuteVerifyHashesThrowsTransitionsToFinalizing)
{
    EXPECT_CALL(*failSep, id())
        .WillOnce(testing::Return(StateExecutor::FINALIZING));

    sm->context.manifest = newManifest;

    auto it = sm->idToStateMap_.find(StateExecutor::VERIFYING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto verifyingExecutor = dynamic_cast<VerifyingStateExecutorSpy*>(it->second.get());
    ASSERT_NE(verifyingExecutor, nullptr);

    auto cryptoMock = static_cast<MockCryptoUtils*>(sm->context.cryptoUtils.get());
    EXPECT_CALL(*cryptoMock, sha256FromFile)
        .WillOnce(Return(std::vector<uint8_t>{1,2,3,1}));

    EXPECT_CALL(*verifyingExecutor, verifyHashes(testing::_, testing::_))
        .WillOnce(testing::Throw(std::runtime_error("Hashes mismatch")));

    EXPECT_CALL(*verifyingExecutor, setEnvVar(testing::_, testing::_))
        .Times(0);

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, dump)
        .Times(1);

    verifyingExecutor->execute(*sm);

    ASSERT_EQ(sm->state(), StateExecutor::FINALIZING);
    ASSERT_TRUE(sm->context.rollback);
    ASSERT_EQ(sm->context.reportMessage.first, ARTIFACT_INTEGRITY_ERROR);
}
