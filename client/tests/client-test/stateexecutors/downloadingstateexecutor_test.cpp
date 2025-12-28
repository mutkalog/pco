#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <tests/client-test/mocks/stateexecutor_mock.h>
#include <tests/client-test/mocks/statepersistence_mock.h>
#include <tests/client-test/mocks/deviceinfo_mock.h>
#include <tests/client-test/mocks/httpclient_mock.h>
#include <tests/client-test/stateexecutors/executorsfixturebase.h>
#include <mocks/archivetools_mock.h>

#include "core/stateexecutors/dowloadingstateexecutor.h"
#include "core/statemachine.h"


namespace {
enum : uint32_t {START_STATE, USUAL_STATE};
} // namespace

using namespace testing;

class DownloadingStateExecutorTest : public DownloadingStateExecutor
{
public:
    using DownloadingStateExecutor::process;
    void sleep(std::chrono::minutes m) override {};

    DownloadingStateExecutorTest() : DownloadingStateExecutor(DOWNLOADING) {}
};

class DownloadingStateExecutorSpy : public DownloadingStateExecutorTest
{
public:
    MOCK_METHOD(void, process, (StateMachine & sm, const std::string &responseBody), (override));
};



class DonwloadingStateExecutorTestFixture : public ExecutorsFixtureBase
{
protected:

    void SetUp() override
    {
        auto mockDevConf          = std::make_unique<NiceMock<MockClientConfig>>();
        auto mockArchiveTools     = std::make_unique<NiceMock<MockArchiveTools>>();
        auto mockStatePersistence = std::make_unique<NiceMock<MockStatePersistence>>();

        targetSe = std::make_unique<NiceMock<MockStateExecutor>>(); targetSep = targetSe.get();
        failSe   = std::make_unique<NiceMock<MockStateExecutor>>(); failSep   = failSe.get();

        std::unordered_map<uint32_t, std::unique_ptr<IStateExecutor>> idToStateMap;
        idToStateMap.emplace(START_STATE, std::make_unique<NiceMock<MockStateExecutor>>());
        idToStateMap.emplace(StateExecutor::DOWNLOADING, std::make_unique<DownloadingStateExecutorTest>());
        idToStateMap.emplace(StateExecutor::VERIFYING, std::move(targetSe));
        idToStateMap.emplace(StateExecutor::FINALIZING, std::move(failSe));

        sm = std::make_unique<StateMachineTestAllPublic>(
            std::move(mockStatePersistence),
            UpdateContext(std::move(mockDevConf), nullptr, nullptr, std::move(mockArchiveTools), nullptr, "", ""),
            std::move(idToStateMap),
            START_STATE,
            StateExecutor::VERIFYING
        );
    }
};

TEST_F(DonwloadingStateExecutorTestFixture, ProcessExtractSuccess)
{
    EXPECT_CALL(*targetSep, id())
        .WillOnce(testing::Return(StateExecutor::VERIFYING));

    auto it = sm->idToStateMap_.find(StateExecutor::DOWNLOADING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto downloadingExecutor = dynamic_cast<DownloadingStateExecutorTest*>(it->second.get());
    ASSERT_NE(downloadingExecutor, nullptr);

    auto archiveMock = static_cast<MockArchiveTools*>(sm->context.archiveTools.get());

    std::string response = "dummy archive data";

    EXPECT_CALL(*archiveMock, extract(testing::_, response.size(), testing::_))
        .WillOnce(testing::Return(0));

    downloadingExecutor->process(*sm, response);

    ASSERT_EQ(sm->state(), StateExecutor::VERIFYING);
    ASSERT_EQ(sm->context.busyResources.stagingDirCreated, 1);
}


TEST_F(DonwloadingStateExecutorTestFixture, ProcessExtractFail)
{
    EXPECT_CALL(*failSep, id())
        .WillOnce(testing::Return(StateExecutor::FINALIZING));

    auto it = sm->idToStateMap_.find(StateExecutor::DOWNLOADING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto downloadingExecutor = dynamic_cast<DownloadingStateExecutorTest*>(it->second.get());
    ASSERT_NE(downloadingExecutor, nullptr);

    auto archiveMock = static_cast<MockArchiveTools*>(sm->context.archiveTools.get());

    std::string response = "dummy archive data";

    EXPECT_CALL(*archiveMock, extract(testing::_, response.size(), testing::_))
        .WillOnce(testing::Return(1));

    downloadingExecutor->process(*sm, response);

    ASSERT_EQ(sm->state(), StateExecutor::FINALIZING);
    ASSERT_EQ(sm->context.busyResources.stagingDirCreated, 0);
    ASSERT_EQ(sm->context.reportMessage.first, INTERNAL_UPDATE_ERROR);
}



class DonwloadingStateExecutorSpyFixture : public DonwloadingStateExecutorTestFixture
{
protected:
    void SetUp() override
    {
        auto mockDevConf          = std::make_unique<NiceMock<MockClientConfig>>();
        auto mockHttpClient       = std::make_unique<NiceMock<MockHttpClient>>();
        auto mockStatePersistence = std::make_unique<NiceMock<MockStatePersistence>>();

        targetSe = std::make_unique<NiceMock<MockStateExecutor>>(); targetSep = targetSe.get();
        failSe   = std::make_unique<NiceMock<MockStateExecutor>>(); failSep   = failSe.get();

        std::unordered_map<uint32_t, std::unique_ptr<IStateExecutor>> idToStateMap;
        idToStateMap.emplace(START_STATE, std::make_unique<NiceMock<MockStateExecutor>>());
        idToStateMap.emplace(StateExecutor::DOWNLOADING, std::make_unique<DownloadingStateExecutorSpy>());
        idToStateMap.emplace(StateExecutor::VERIFYING, std::move(targetSe));
        idToStateMap.emplace(StateExecutor::FINALIZING, std::move(failSe));

        sm = std::make_unique<StateMachineTestAllPublic>(
            std::move(mockStatePersistence),
            UpdateContext(std::move(mockDevConf), std::move(mockHttpClient), nullptr, nullptr, nullptr, "", ""),
            std::move(idToStateMap),
            START_STATE,
            StateExecutor::VERIFYING
        );
    }
};


TEST_F(DonwloadingStateExecutorSpyFixture, ExecuteServerReturns200CallsProcess)
{
    auto it = sm->idToStateMap_.find(StateExecutor::DOWNLOADING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto downloadingExecutor = dynamic_cast<DownloadingStateExecutorSpy*>(it->second.get());
    ASSERT_NE(downloadingExecutor, nullptr);

    EXPECT_CALL(*downloadingExecutor, process)
        .Times(1);

    auto response = std::make_unique<httplib::Response>();
    response->status = httplib::OK_200;
    response->body   = "dummy download data";
    httplib::Result result(std::move(response), httplib::Error::Success);

    auto httpMock = static_cast<MockHttpClient*>(sm->context.client.get());
    EXPECT_CALL(*httpMock, Get)
        .WillOnce(testing::Return(std::move(result)));

    downloadingExecutor->execute(*sm);
}


TEST_F(DonwloadingStateExecutorSpyFixture, ExecuteServerUnavailableThrowsAndFinalizing)
{
    EXPECT_CALL(*failSep, id())
        .WillOnce(testing::Return(StateExecutor::FINALIZING));

    auto it = sm->idToStateMap_.find(StateExecutor::DOWNLOADING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto downloadingExecutor = dynamic_cast<DownloadingStateExecutorSpy*>(it->second.get());
    ASSERT_NE(downloadingExecutor, nullptr);

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, dump)
        .Times(1);

    auto response = std::make_unique<httplib::Response>();
    httplib::Result result(nullptr, httplib::Error::Success);

    auto httpMock = static_cast<MockHttpClient*>(sm->context.client.get());
    EXPECT_CALL(*httpMock, Get(_))
        .Times(4)
        .WillRepeatedly([](const std::string &) {
            return httplib::Result(nullptr, httplib::Error::Connection);
        });

    downloadingExecutor->execute(*sm);

    ASSERT_EQ(sm->state(), StateExecutor::FINALIZING);
    ASSERT_TRUE(sm->context.rollback);
}



