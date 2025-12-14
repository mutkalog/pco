#ifndef VERIFYINGSTATEEXECUTOR_H
#define VERIFYINGSTATEEXECUTOR_H

#include "../statemachine.h"

class VerifyingStateExecutor final : public StateExecutor
{
public:
    static VerifyingStateExecutor& instance();
    virtual void execute(StateMachine& sm) override;

private:
    VerifyingStateExecutor(enum StateId id) : StateExecutor(id) {}
};

inline VerifyingStateExecutor &VerifyingStateExecutor::instance()
{
    static VerifyingStateExecutor inst(VERIFYING);
    return inst;
}

#endif // VERIFYINGSTATEEXECUTOR_H
