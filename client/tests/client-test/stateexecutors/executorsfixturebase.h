#ifndef EXECUTORSFIXTUREBASE_H
#define EXECUTORSFIXTUREBASE_H

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include "core/statemachine.h"
#include "tests/client-test/mocks/stateexecutor_mock.h"

class ExecutorsFixtureBase : public testing::Test
{
protected:
    class StateMachineTestAllPublic : public StateMachine
    {
    public:
        using StateMachine::recover;
        using StateMachine::StateMachine;
        using StateMachine::inCriticalStates_;
        using StateMachine::idToStateMap_;
        using StateMachine::sp_;
    };

    std::unique_ptr<MockStateExecutor> targetSe;
    std::unique_ptr<MockStateExecutor> failSe;
    MockStateExecutor* targetSep;
    MockStateExecutor* failSep;

    std::unique_ptr<StateMachineTestAllPublic> sm;
};
#endif // EXECUTORSFIXTUREBASE_H
