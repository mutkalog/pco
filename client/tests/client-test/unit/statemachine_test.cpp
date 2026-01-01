#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include "mocks/statepersistence_mock.h"
#include "mocks/stateexecutor_mock.h"
#include "core/statemachine.h"


class StateMachineTest : public testing::Test
{
protected:
    class StateMachineTestAllPublic : public StateMachine
    {
    public:
        using StateMachine::recover;
        using StateMachine::StateMachine;
        using StateMachine::inCriticalStates_;
    };

    std::unique_ptr<StateMachineTestAllPublic> sm;

    MockStateExecutor* se1p;
    MockStateExecutor* se2p;
    MockStateExecutor* se3p;
    MockStatePersistence* spp;

    enum : uint32_t {START_STATE, USUAL_STATE, CRITICAL_STATE};

    void SetUp() override
    {
        auto se1up = std::make_unique<MockStateExecutor>();    se1p = se1up.get();
        auto se2up = std::make_unique<MockStateExecutor>();    se2p = se2up.get();
        auto se3up = std::make_unique<MockStateExecutor>();    se3p = se3up.get();
        auto spup  = std::make_unique<MockStatePersistence>(); spp  = spup.get();

        std::unordered_map<uint32_t, std::unique_ptr<IStateExecutor>> idToStateMap;
        idToStateMap.emplace(START_STATE, std::move(se1up));
        idToStateMap.emplace(USUAL_STATE, std::move(se2up));
        idToStateMap.emplace(CRITICAL_STATE, std::move(se3up));

        sm = std::make_unique<StateMachineTestAllPublic>(
                std::move(spup),
                UpdateContext(nullptr, nullptr, nullptr, nullptr, nullptr, "", ""),
                std::move(idToStateMap),
                START_STATE, CRITICAL_STATE);
    }
};


TEST_F(StateMachineTest, RunSuccess)
{
    EXPECT_CALL(*se1p, execute).Times(1);
    sm->run();
}


TEST_F(StateMachineTest, TransitToNonCriticalState)
{
    EXPECT_CALL(*spp, clear).Times(1);
    EXPECT_CALL(*spp, dump).Times(0);

    EXPECT_CALL(*se2p, id()).WillOnce(testing::Return(USUAL_STATE));
    EXPECT_CALL(*se2p, textId()).WillOnce(testing::Return("USUAL_STATE"));

    sm->transitTo(USUAL_STATE);
    ASSERT_EQ(sm->state(), USUAL_STATE);
}


TEST_F(StateMachineTest, TransitToCriticalState)
{
    EXPECT_CALL(*spp, dump).Times(1);
    EXPECT_CALL(*spp, clear).Times(0);

    EXPECT_CALL(*se3p, id()).WillOnce(testing::Return(CRITICAL_STATE));
    EXPECT_CALL(*se3p, textId()).WillOnce(testing::Return("CRITICAL_STATE"));

    sm->transitTo(CRITICAL_STATE);
    ASSERT_EQ(sm->state(), CRITICAL_STATE);
}


TEST_F(StateMachineTest, TransitToFullCycle)
{
    EXPECT_CALL(*spp, clear).Times(1);
    EXPECT_CALL(*spp, dump).Times(0);
    EXPECT_CALL(*se2p, id()).WillOnce(testing::Return(USUAL_STATE));
    EXPECT_CALL(*se2p, textId()).WillOnce(testing::Return("USUAL_STATE"));
    sm->transitTo(USUAL_STATE);
    ASSERT_EQ(sm->state(), USUAL_STATE);

    EXPECT_CALL(*spp, dump).Times(1);
    EXPECT_CALL(*spp, clear).Times(0);
    EXPECT_CALL(*se3p, id()).WillOnce(testing::Return(CRITICAL_STATE));
    EXPECT_CALL(*se3p, textId()).WillOnce(testing::Return("CRITICAL_STATE"));
    sm->transitTo(CRITICAL_STATE);
    ASSERT_EQ(sm->state(), CRITICAL_STATE);

    EXPECT_CALL(*spp, clear).Times(1);
    EXPECT_CALL(*spp, dump).Times(0);
    EXPECT_CALL(*se1p, id()).WillOnce(testing::Return(START_STATE));
    EXPECT_CALL(*se1p, textId()).WillOnce(testing::Return("START_STATE"));
    sm->transitTo(START_STATE);
    ASSERT_EQ(sm->state(), START_STATE);
}


TEST_F(StateMachineTest, RecoverNoStateFile)
{
    EXPECT_CALL(*spp, load(testing::_)).
        WillOnce(testing::Return(std::nullopt));
    EXPECT_CALL(*se1p, id())
        .WillOnce(testing::Return(START_STATE));

    sm->recover();

    ASSERT_EQ(sm->state(), START_STATE);
    ASSERT_FALSE(sm->inCriticalStates_);
    ASSERT_FALSE(sm->context.recovering);
}


TEST_F(StateMachineTest, RecoverStateFileExists)
{
    EXPECT_CALL(*spp, load(testing::_)).
        WillOnce(testing::Return(CRITICAL_STATE));
    EXPECT_CALL(*se3p, id())
        .WillOnce(testing::Return(CRITICAL_STATE));

    sm->recover();

    ASSERT_EQ(sm->state(), CRITICAL_STATE);
    ASSERT_TRUE(sm->inCriticalStates_);
    ASSERT_TRUE(sm->context.recovering);
}

