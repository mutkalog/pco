#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <mocks/cryptoutils_mock.h>
#include <tests/client-test/unit/mocks/stateexecutor_mock.h>
#include <tests/client-test/unit/mocks/statepersistence_mock.h>
#include <tests/client-test/unit/mocks/deviceinfo_mock.h>
#include <tests/client-test/unit/mocks/syscalls_mock.h>
#include <tests/client-test/unit/mocks/httpclient_mock.h>
#include <tests/client-test/unit/stateexecutors/executorsfixturebase.h>

#include "core/stateexecutors/preparingstateexecutor.h"


namespace {
const std::string stagingDir = "/tmp/pco/";
enum : uint32_t {START_STATE, USUAL_STATE};
} // namespace


using namespace testing;

class PreparingStateExecutorAllPublic : public PreparingStateExecutor
{
public:
    using PreparingStateExecutor::PreparingStateExecutor;
};


class PreparingStateExecutorTestFixture : public ExecutorsFixtureBase
{
protected:
    void SetUp() override
    {
        auto mockDevConf          = std::make_unique<NiceMock<MockClientConfig>>();
        auto mockStatePersistence = std::make_unique<NiceMock<MockStatePersistence>>();
        auto mockCryptoUtils      = std::make_unique<NiceMock<MockCryptoUtils>>();
        auto mockSyscalls         = std::make_unique<NiceMock<MockSystemCalls>>();

        targetSe = std::make_unique<NiceMock<MockStateExecutor>>(); targetSep = targetSe.get();
        failSe   = std::make_unique<NiceMock<MockStateExecutor>>(); failSep   = failSe.get();

        auto prepSe = std::make_unique<PreparingStateExecutorAllPublic>(StateExecutor::PREPARING);

        std::unordered_map<uint32_t, std::unique_ptr<IStateExecutor>> idToStateMap;
        idToStateMap.emplace(START_STATE, std::make_unique<NiceMock<MockStateExecutor>>());
        idToStateMap.emplace(StateExecutor::PREPARING,  std::move(prepSe));
        idToStateMap.emplace(StateExecutor::INSTALLING, std::move(targetSe));
        idToStateMap.emplace(StateExecutor::FINALIZING, std::move(failSe));

        sm = std::make_unique<StateMachineTestAllPublic>(
            std::move(mockStatePersistence),
            UpdateContext(std::move(mockDevConf), nullptr,
                          std::move(mockCryptoUtils), nullptr,
                          std::move(mockSyscalls), stagingDir, ""),
            std::move(idToStateMap), START_STATE, StateExecutor::VERIFYING);
    }
};


TEST_F(PreparingStateExecutorTestFixture, ExecuteSuccessTransitsToInstalling)
{
    EXPECT_CALL(*targetSep, id())
        .WillOnce(testing::Return(StateExecutor::INSTALLING));

    auto it = sm->idToStateMap_.find(StateExecutor::PREPARING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto prepairingExecutor = dynamic_cast<PreparingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(prepairingExecutor, nullptr);

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

    prepairingExecutor->execute(*sm);

    ASSERT_EQ(sm->state(), StateExecutor::INSTALLING);
    ASSERT_FALSE(sm->context.rollback);
}


TEST_F(PreparingStateExecutorTestFixture, ExecuteChmodFailsTransitsToFinalizing)
{
    EXPECT_CALL(*failSep, id())
        .WillOnce(testing::Return(StateExecutor::FINALIZING));

    auto it = sm->idToStateMap_.find(StateExecutor::PREPARING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto preparingExecutor = dynamic_cast<PreparingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(preparingExecutor, nullptr);

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, dump)
        .Times(1);

    auto mockSys = static_cast<MockSystemCalls*>(sm->context.syscalls.get());
    EXPECT_CALL(*mockSys, chmod(_, 0755))
        .WillOnce(testing::Return(-1));

    preparingExecutor->execute(*sm);

    ASSERT_EQ(sm->state(), StateExecutor::FINALIZING);
    ASSERT_TRUE(sm->context.rollback);
    ASSERT_EQ(sm->context.reportMessage.first, INTERNAL_UPDATE_ERROR);
}


TEST_F(PreparingStateExecutorTestFixture, ExecuteSpawnFailsTransitsToFinalizing)
{
    EXPECT_CALL(*failSep, id())
        .WillOnce(testing::Return(StateExecutor::FINALIZING));

    auto it = sm->idToStateMap_.find(StateExecutor::PREPARING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto preparingExecutor = dynamic_cast<PreparingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(preparingExecutor, nullptr);

    auto spMock = static_cast<MockStatePersistence*>(sm->sp_.get());
    EXPECT_CALL(*spMock, dump)
        .Times(1);

    auto mockSys = static_cast<MockSystemCalls*>(sm->context.syscalls.get());
    EXPECT_CALL(*mockSys, chmod(_, 0755))
        .WillOnce(testing::Return(0));

    EXPECT_CALL(*mockSys, posix_spawn(_, _, _, _, _, _))
        .WillOnce(testing::Return(-1));

    preparingExecutor->execute(*sm);

    ASSERT_EQ(sm->state(), StateExecutor::FINALIZING);
    ASSERT_TRUE(sm->context.rollback);
    ASSERT_EQ(sm->context.reportMessage.first, INTERNAL_UPDATE_ERROR);
}


TEST_F(PreparingStateExecutorTestFixture, ExecuteWaitpidReturnsNonZeroExitTransitsToFinalizing)
{
    EXPECT_CALL(*failSep, id())
        .WillOnce(testing::Return(StateExecutor::FINALIZING));

    auto it = sm->idToStateMap_.find(StateExecutor::PREPARING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto preparingExecutor = dynamic_cast<PreparingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(preparingExecutor, nullptr);

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

    preparingExecutor->execute(*sm);

    ASSERT_EQ(sm->state(), StateExecutor::FINALIZING);
    ASSERT_TRUE(sm->context.rollback);
    ASSERT_EQ(sm->context.reportMessage.first, INTERNAL_UPDATE_ERROR);
}


TEST_F(PreparingStateExecutorTestFixture, ExecuteWaitpidExitNonZeroTransitsToFinalizing)
{
    EXPECT_CALL(*failSep, id())
        .WillOnce(testing::Return(StateExecutor::FINALIZING));

    auto it = sm->idToStateMap_.find(StateExecutor::PREPARING);
    ASSERT_NE(it, sm->idToStateMap_.end());
    auto preparingExecutor = dynamic_cast<PreparingStateExecutorAllPublic*>(it->second.get());
    ASSERT_NE(preparingExecutor, nullptr);

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

    preparingExecutor->execute(*sm);

    ASSERT_EQ(sm->state(), StateExecutor::FINALIZING);
    ASSERT_TRUE(sm->context.rollback);
    ASSERT_EQ(sm->context.reportMessage.first, INTERNAL_UPDATE_ERROR);
    ASSERT_TRUE(sm->context.reportMessage.second.find("prepare.sh returned 1") != std::string::npos);
}
