#ifndef PREPARINGSTATEEXECUTOR_H
#define PREPARINGSTATEEXECUTOR_H

#include "../statemachine.h"
#include "stateexecutor.h"

class PreparingStateExecutor final : public StateExecutor
{
public:
    static PreparingStateExecutor& instance();
    virtual void execute(StateMachine& sm) override;

public:
    PreparingStateExecutor(enum StateId id) : StateExecutor(id) {}
};

inline PreparingStateExecutor &PreparingStateExecutor::instance()
{
    static PreparingStateExecutor inst(PREPARING);
    return inst;
}

#endif // PREPARINGSTATEEXECUTOR_H
