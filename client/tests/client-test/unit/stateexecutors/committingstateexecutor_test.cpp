#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <mocks/cryptoutils_mock.h>
#include <tests/client-test/unit/mocks/stateexecutor_mock.h>
#include <tests/client-test/unit/mocks/statepersistence_mock.h>
#include <tests/client-test/unit/mocks/deviceinfo_mock.h>
#include <tests/client-test/unit/mocks/syscalls_mock.h>
#include <tests/client-test/unit/mocks/httpclient_mock.h>
#include <tests/client-test/unit/stateexecutors/executorsfixturebase.h>

#include "core/stateexecutors/committingstateexecutor.h"


namespace {
const std::string stagingDir = "/tmp/pco/";
enum : uint32_t {START_STATE, USUAL_STATE};
} // namespace


using namespace testing;

class CommittingStateExecutorAllPublic : public CommittingStateExecutor
{
public:
    using CommittingStateExecutor::CommittingStateExecutor;
};


class CommittingStateExecutorTestFixture : public ExecutorsFixtureBase
{
protected:
    void SetUp() override
    {
        auto mockDevConf          = std::make_unique<NiceMock<MockClientConfig>>();
        auto mockStatePersistence = std::make_unique<NiceMock<MockStatePersistence>>();
        auto mockCryptoUtils      = std::make_unique<NiceMock<MockCryptoUtils>>();
        auto mockSyscalls         = std::make_unique<NiceMock<MockSystemCalls>>();

        targetSe = std::make_unique<MockStateExecutor>(); targetSep = targetSe.get();
        failSe   = std::make_unique<MockStateExecutor>(); failSep   = failSe.get();

        auto cmtSe = std::make_unique<CommittingStateExecutorAllPublic> (StateExecutor::COMMITTING);

        std::unordered_map<uint32_t, std::unique_ptr<IStateExecutor>> idToStateMap;
        idToStateMap.emplace(START_STATE, std::make_unique<NiceMock<MockStateExecutor>>());
        idToStateMap.emplace(StateExecutor::COMMITTING, std::move(cmtSe));
        idToStateMap.emplace(StateExecutor::FINALIZING, std::move(targetSe));

        sm = std::make_unique<StateMachineTestAllPublic>(
            std::move(mockStatePersistence),
            UpdateContext(std::move(mockDevConf), nullptr,
                          std::move(mockCryptoUtils), nullptr,
                          std::move(mockSyscalls), stagingDir, ""),
            std::move(idToStateMap),
            START_STATE, StateExecutor::VERIFYING);
    }
};


TEST_F(CommittingStateExecutorTestFixture, ExecuteSuccessTransitsToFINALIZING)
{
    EXPECT_CALL(*targetSep, id())
        .WillOnce(testing::Return(StateExecutor::FINALIZING));

    auto it = sm->idToStateMap_.find(StateExecutor::COMMITTING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto committingExecutor = dynamic_cast<CommittingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(committingExecutor, nullptr);

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, dump)
        .Times(1);

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

    committingExecutor->execute(*sm);

    ASSERT_EQ(sm->state(), StateExecutor::FINALIZING);
    ASSERT_FALSE(sm->context.rollback);
}


TEST_F(CommittingStateExecutorTestFixture, ExecuteChmodFailsTransitsToFinalizing)
{
    EXPECT_CALL(*targetSep, id())
        .WillOnce(testing::Return(StateExecutor::FINALIZING));

    auto it = sm->idToStateMap_.find(StateExecutor::COMMITTING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto CommittingExecutor = dynamic_cast<CommittingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(CommittingExecutor, nullptr);

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, dump)
        .Times(1);

    auto mockSys = static_cast<MockSystemCalls*>(sm->context.syscalls.get());
    EXPECT_CALL(*mockSys, chmod(_, 0755))
        .WillOnce(testing::Return(-1));

    CommittingExecutor->execute(*sm);

    ASSERT_EQ(sm->state(), StateExecutor::FINALIZING);
    ASSERT_TRUE(sm->context.rollback);
    ASSERT_EQ(sm->context.reportMessage.first, INTERNAL_UPDATE_ERROR);
}


TEST_F(CommittingStateExecutorTestFixture, ExecuteSpawnFailsTransitsToFinalizing)
{
    EXPECT_CALL(*targetSep, id())
        .WillOnce(testing::Return(StateExecutor::FINALIZING));

    auto it = sm->idToStateMap_.find(StateExecutor::COMMITTING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto committingExecutor = dynamic_cast<CommittingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(committingExecutor, nullptr);

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, dump)
        .Times(1);

    auto mockSys = static_cast<MockSystemCalls*>(sm->context.syscalls.get());
    EXPECT_CALL(*mockSys, chmod(_, 0755))
        .WillOnce(testing::Return(0));

    EXPECT_CALL(*mockSys, posix_spawn(_, _, _, _, _, _))
        .WillOnce(testing::Return(-1));

    committingExecutor->execute(*sm);

    ASSERT_EQ(sm->state(), StateExecutor::FINALIZING);
    ASSERT_TRUE(sm->context.rollback);
    ASSERT_EQ(sm->context.reportMessage.first, INTERNAL_UPDATE_ERROR);
}


TEST_F(CommittingStateExecutorTestFixture, ExecuteWaitpidReturnsNonZeroExitTransitsToFinalizing)
{
    EXPECT_CALL(*targetSep, id())
        .WillOnce(testing::Return(StateExecutor::FINALIZING));

    auto it = sm->idToStateMap_.find(StateExecutor::COMMITTING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto CommittingExecutor = dynamic_cast<CommittingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(CommittingExecutor, nullptr);

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, dump)
        .Times(1);

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
            *status = 1;
            return 1234;
        });

    CommittingExecutor->execute(*sm);

    ASSERT_EQ(sm->state(), StateExecutor::FINALIZING);
    ASSERT_TRUE(sm->context.rollback);
    ASSERT_EQ(sm->context.reportMessage.first, INTERNAL_UPDATE_ERROR);
}


TEST_F(CommittingStateExecutorTestFixture, ExecuteWaitpidExitNonZeroTransitsToFinalizing)
{
    EXPECT_CALL(*targetSep, id())
        .WillOnce(testing::Return(StateExecutor::FINALIZING));

    auto it = sm->idToStateMap_.find(StateExecutor::COMMITTING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto CommittingExecutor = dynamic_cast<CommittingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(CommittingExecutor, nullptr);

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, dump)
        .Times(1);

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
            *status = 0x0100; // WIFEXITED(status) == true, но WEXITSTATUS(status) == 1
            return 1234;
        });

    CommittingExecutor->execute(*sm);

    ASSERT_EQ(sm->state(), StateExecutor::FINALIZING);
    ASSERT_TRUE(sm->context.rollback);
    ASSERT_EQ(sm->context.reportMessage.first, INTERNAL_UPDATE_ERROR);
}
