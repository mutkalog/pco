#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <mocks/cryptoutils_mock.h>
#include <tests/client-test/mocks/stateexecutor_mock.h>
#include <tests/client-test/mocks/statepersistence_mock.h>
#include <tests/client-test/mocks/deviceinfo_mock.h>
#include <tests/client-test/mocks/httpclient_mock.h>

#include "core/stateexecutors/checkingstateexecutor.h"
#include "core/artifactmanifest.h"
#include "core/statemachine.h"

#include "executorsfixturebase.h"


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
using ::testing::NiceMock;

class CheckingStateExecutorTest : public CheckingStateExecutor
{
public:
    using CheckingStateExecutor::process;
    using CheckingStateExecutor::verificateRelease;
    using CheckingStateExecutor::compareVersions;
    using CheckingStateExecutor::compareDeviceType;

    CheckingStateExecutorTest() : CheckingStateExecutor(CHECKING) {}
};

class CheckingStateExecutorSpy : public CheckingStateExecutorTest
{
public:
    MOCK_METHOD(void, process, (StateMachine & sm, const std::string &responseBody), (override));
};


TEST(CheckingStateExecutorTest, CompareVersionsSuccess)
{
    CheckingStateExecutorTest cs;

    std::string received = "2.3.45";
    std::string current  = "2.4.0";
    ASSERT_TRUE(cs.compareVersions(received, current));
}


TEST(CheckingStateExecutorTest, CompareVersionsFail)
{
    CheckingStateExecutorTest cs;

    std::string received = "1.0.0";
    std::string current  = "1.0.0";
    ASSERT_FALSE(cs.compareVersions(received, current));
}


TEST(CheckingStateExecutorTest, CompareDeviceTypeTestSuccessFullData)
{
    CheckingStateExecutorTest cs;
    ArtifactManifest received, current;

    received.release.type     = "Orange Pi 2";
    received.release.platform = "Debian 11";
    received.release.arch     = "ARMv7";

    current.release.type      = "Orange Pi 2";
    current.release.platform  = "Debian 11";
    current.release.arch      = "ARMv7";

    ASSERT_TRUE(cs.compareDeviceType(received, current));
}


TEST(CheckingStateExecutorTest, CompareDeviceTypeTestSuccessFirstUpdate)
{
    CheckingStateExecutorTest cs;
    ArtifactManifest received, current;

    received.release.type     = "Orange Pi 2";
    received.release.platform = "Debian 11";
    received.release.arch     = "ARMv7";

    ASSERT_TRUE(cs.compareDeviceType(received, current));
}


TEST(CheckingStateExecutorTest, CompareDeviceTypeTestFailArchDiffers)
{
    CheckingStateExecutorTest cs;
    ArtifactManifest received, current;

    received.release.type     = "Orange Pi 2";
    received.release.platform = "Debian 11";
    received.release.arch     = "ARMv7";

    current.release.type      = "Orange Pi 2";
    current.release.platform  = "Debian 11";
    current.release.arch      = "ARMvvv7";

    ASSERT_FALSE(cs.compareDeviceType(received, current));
}

class CheckingStateExecutorTestFixture : public ExecutorsFixtureBase
{
protected:
    std::string responseBody = R"(
    {
        "manifest": "{ \"release\": { \"version\": \"1.3.0\", \"type\": \"Raspberry Pi 4\", \"timestamp\": \"2025-11-12T10:23:00Z\", \"platform\": \"Linux\", \"arch\": \"ARMv8\" }, \"files\": [] }",
        "signature": "{ \"signature\": { \"algo\": \"rsa-sha256\", \"keyname\": \"main-signing-key\", \"value\": \"dummy-signature\" } }"
    })";

    void SetUp() override
    {
        auto mockDevConf          = std::make_unique<NiceMock<MockClientConfig>>();
        auto mockCryptoUtils      = std::make_unique<NiceMock<MockCryptoUtils>>();
        auto mockHttpClient       = std::make_unique<NiceMock<MockHttpClient>>();
        auto mockStatePersistence = std::make_unique<NiceMock<MockStatePersistence>>();

        targetSe = std::make_unique<MockStateExecutor>(); targetSep = targetSe.get();
        failSe   = std::make_unique<MockStateExecutor>(); failSep   = failSe.get();

        std::unordered_map<uint32_t, std::unique_ptr<IStateExecutor>> idToStateMap;
        idToStateMap.emplace(START_STATE, std::make_unique<NiceMock<MockStateExecutor>>());
        idToStateMap.emplace(StateExecutor::IDLE, std::move(failSe));
        idToStateMap.emplace(StateExecutor::CHECKING, std::make_unique<CheckingStateExecutorTest>());
        idToStateMap.emplace(StateExecutor::DOWNLOADING, std::move(targetSe));

        sm = std::make_unique<StateMachineTestAllPublic>(
            std::move(mockStatePersistence),
            UpdateContext(std::move(mockDevConf), std::move(mockHttpClient), std::move(mockCryptoUtils), nullptr, nullptr, "", ""),
            std::move(idToStateMap),
            START_STATE,
            StateExecutor::VERIFYING
        );
    }

};


TEST_F(CheckingStateExecutorTestFixture, ProcessTransitToDownloadingState)
{
    EXPECT_CALL(*targetSep, id())
        .WillOnce(testing::Return(StateExecutor::DOWNLOADING));

    auto devMock = static_cast<MockClientConfig*>(sm->context.devconf.get());
    EXPECT_CALL(*devMock, prevManifest())
        .WillRepeatedly(testing::Return(oldManifest));

    auto cryptoMock = static_cast<MockCryptoUtils*>(sm->context.cryptoUtils.get());
    EXPECT_CALL(*cryptoMock, verifySignature)
        .WillOnce(testing::Return(true));

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, clear())
        .Times(1);

    auto it = sm->idToStateMap_.find(StateExecutor::CHECKING);
    ASSERT_NE(it, sm->idToStateMap_.end());

    auto* checkingExecutor = dynamic_cast<CheckingStateExecutorTest*>(it->second.get());
    ASSERT_NE(checkingExecutor, nullptr);

    checkingExecutor->process(*sm, responseBody);

    ASSERT_EQ(StateExecutor::DOWNLOADING, sm->state());
}


TEST_F(CheckingStateExecutorTestFixture, ProcessTransitToIdleStateSignatureFail)
{
    EXPECT_CALL(*failSep, id())
        .WillOnce(testing::Return(StateExecutor::IDLE));

    auto devMock = static_cast<MockClientConfig*>(sm->context.devconf.get());
    EXPECT_CALL(*devMock, prevManifest())
        .Times(0);

    auto cryptoMock = static_cast<MockCryptoUtils*>(sm->context.cryptoUtils.get());
    EXPECT_CALL(*cryptoMock, verifySignature)
        .WillOnce(testing::Return(false));

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, clear())
        .Times(1);

    auto it = sm->idToStateMap_.find(StateExecutor::CHECKING);
    ASSERT_NE(it, sm->idToStateMap_.end());

    auto* checkingExecutor = dynamic_cast<CheckingStateExecutorTest*>(it->second.get());
    ASSERT_NE(checkingExecutor, nullptr);

    checkingExecutor->process(*sm, responseBody);

    ASSERT_EQ(StateExecutor::IDLE, sm->state());
}


TEST_F(CheckingStateExecutorTestFixture, ProcessTransitToIdleStateReleaseFail)
{
    EXPECT_CALL(*failSep, id())
        .WillOnce(testing::Return(StateExecutor::IDLE));

    auto devMock = static_cast<MockClientConfig*>(sm->context.devconf.get());
    EXPECT_CALL(*devMock, prevManifest())
        .WillRepeatedly(testing::Return(newManifest));

    auto cryptoMock = static_cast<MockCryptoUtils*>(sm->context.cryptoUtils.get());
    EXPECT_CALL(*cryptoMock, verifySignature)
        .WillOnce(testing::Return(true));

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, clear())
        .Times(1);

    auto it = sm->idToStateMap_.find(StateExecutor::CHECKING);
    ASSERT_NE(it, sm->idToStateMap_.end());

    auto* checkingExecutor = dynamic_cast<CheckingStateExecutorTest*>(it->second.get());
    ASSERT_NE(checkingExecutor, nullptr);

    checkingExecutor->process(*sm, responseBody);

    ASSERT_EQ(StateExecutor::IDLE, sm->state());
}

class CheckingStateExecutorSpyFixture : public CheckingStateExecutorTestFixture
{
protected:
    void SetUp() override
    {
        auto mockDevConf          = std::make_unique<NiceMock<MockClientConfig>>();
        auto mockCryptoUtils      = std::make_unique<NiceMock<MockCryptoUtils>>();
        auto mockHttpClient       = std::make_unique<NiceMock<MockHttpClient>>();
        auto mockStatePersistence = std::make_unique<NiceMock<MockStatePersistence>>();

        targetSe = std::make_unique<NiceMock<MockStateExecutor>>(); targetSep = targetSe.get();
        failSe   = std::make_unique<NiceMock<MockStateExecutor>>(); failSep   = failSe.get();

        std::unordered_map<uint32_t, std::unique_ptr<IStateExecutor>> idToStateMap;
        idToStateMap.emplace(START_STATE, std::make_unique<NiceMock<MockStateExecutor>>());
        idToStateMap.emplace(StateExecutor::IDLE, std::move(failSe));
        idToStateMap.emplace(StateExecutor::CHECKING, std::make_unique<CheckingStateExecutorSpy>());
        idToStateMap.emplace(StateExecutor::DOWNLOADING, std::move(targetSe));

        sm = std::make_unique<StateMachineTestAllPublic>(
            std::move(mockStatePersistence),
            UpdateContext(std::move(mockDevConf), std::move(mockHttpClient), std::move(mockCryptoUtils), nullptr, nullptr, "", ""),
            std::move(idToStateMap),
            START_STATE,
            StateExecutor::VERIFYING
        );
    }
};


TEST_F(CheckingStateExecutorSpyFixture, ExecuteServerReturns200AndProcessCalled)
{
    auto response = std::make_unique<httplib::Response>();
    response->status = httplib::OK_200;
    response->body   = responseBody;
    httplib::Result result(std::move(response), httplib::Error::Success);

    auto httpMock = static_cast<MockHttpClient*>(sm->context.client.get());
    EXPECT_CALL(*httpMock, Get)
        .WillOnce(testing::Return(std::move(result)));

    auto it = sm->idToStateMap_.find(StateExecutor::CHECKING);
    ASSERT_NE(it, sm->idToStateMap_.end());

    auto checkingExecutor = dynamic_cast<CheckingStateExecutorSpy*>(it->second.get());
    ASSERT_NE(checkingExecutor, nullptr);

    EXPECT_CALL(*checkingExecutor, process)
        .Times(1);

    checkingExecutor->execute(*sm);
}


TEST_F(CheckingStateExecutorSpyFixture, ExecuteServerReturnsNullAndThenTransitToIdle)
{
    EXPECT_CALL(*failSep, id())
        .WillOnce(testing::Return(StateExecutor::IDLE));

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, clear())
        .Times(1);

    auto response = nullptr;
    httplib::Result result(nullptr, httplib::Error::Unknown);

    auto httpMock = static_cast<MockHttpClient*>(sm->context.client.get());
    EXPECT_CALL(*httpMock, Get)
        .WillOnce(testing::Return(std::move(result)));

    auto it = sm->idToStateMap_.find(StateExecutor::CHECKING);
    ASSERT_NE(it, sm->idToStateMap_.end());

    auto* checkingExecutor = dynamic_cast<CheckingStateExecutorTest*>(it->second.get());
    ASSERT_NE(checkingExecutor, nullptr);

    checkingExecutor->execute(*sm);

    ASSERT_EQ(StateExecutor::IDLE, sm->state());
}


TEST_F(CheckingStateExecutorSpyFixture, ExecuteServerReturnsNot200AndThenTransitToIdle)
{
    EXPECT_CALL(*failSep, id())
        .WillOnce(testing::Return(StateExecutor::IDLE));

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, clear())
        .Times(1);

    auto response = std::make_unique<httplib::Response>();
    response->status = httplib::BadRequest_400;
    response->body   = responseBody;
    httplib::Result result(std::move(response), httplib::Error::Success);

    auto httpMock = static_cast<MockHttpClient*>(sm->context.client.get());
    EXPECT_CALL(*httpMock, Get)
        .WillOnce(testing::Return(std::move(result)));

    auto it = sm->idToStateMap_.find(StateExecutor::CHECKING);
    ASSERT_NE(it, sm->idToStateMap_.end());

    auto* checkingExecutor = dynamic_cast<CheckingStateExecutorTest*>(it->second.get());
    ASSERT_NE(checkingExecutor, nullptr);

    checkingExecutor->execute(*sm);

    ASSERT_EQ(StateExecutor::IDLE, sm->state());
}
