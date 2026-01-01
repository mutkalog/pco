#ifndef IDLESTATEEXECUTOR_H
#define IDLESTATEEXECUTOR_H

#include "stateexecutor.h"
#include "../statemachine.h"


class IdleStateExecutor final : public StateExecutor
{
public:
    virtual void execute(StateMachine& sm) override;
    IdleStateExecutor(enum StateId id) : StateExecutor(id) {}
};

#endif // IDLESTATEEXECUTOR_H
