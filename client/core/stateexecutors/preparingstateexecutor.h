#ifndef PREPARINGSTATEEXECUTOR_H
#define PREPARINGSTATEEXECUTOR_H

#include "core/statemachine.h"
#include "core/stateexecutors/stateexecutor.h"


class PreparingStateExecutor : public StateExecutor
{
public:
    virtual void execute(StateMachine& sm) override;
    PreparingStateExecutor(enum StateId id) : StateExecutor(id) {}
};

#endif // PREPARINGSTATEEXECUTOR_H
