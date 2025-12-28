#ifndef COMMITTINGSTATEEXECUTOR_H
#define COMMITTINGSTATEEXECUTOR_H

#include "core/statemachine.h"


class CommittingStateExecutor : public StateExecutor
{
public:
    virtual void execute(StateMachine& sm) override;
    CommittingStateExecutor(enum StateId id) : StateExecutor(id) {}
};
#endif // COMMITTINGSTATEEXECUTOR_H
