#ifndef TESTINGSTATEEXECUTOR_H
#define TESTINGSTATEEXECUTOR_H

#include "stateexecutor.h"

class TestingStateExecutor final : public StateExecutor
{
public:
    static TestingStateExecutor& instance();
    virtual void execute(StateMachine& sm) override;

private:
    TestingStateExecutor(enum StateId id) : StateExecutor(id) {}
};

inline TestingStateExecutor &TestingStateExecutor::instance()
{
    static TestingStateExecutor inst(PRE_COMMITTING);
    return inst;
}

#endif // TESTINGSTATEEXECUTOR_H
