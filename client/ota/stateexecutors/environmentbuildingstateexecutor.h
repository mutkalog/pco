#ifndef ENVIRONMENTBUILDINGSTATEEXECUTOR_H
#define ENVIRONMENTBUILDINGSTATEEXECUTOR_H

#include "../statemachine.h"
#include "stateexecutor.h"

class PreInstallScriptStateExecutor final : public StateExecutor
{
public:
    static PreInstallScriptStateExecutor& instance();
    virtual void execute(StateMachine& sm) override;

public:
    PreInstallScriptStateExecutor(enum StateId id) : StateExecutor(id) {}
};

inline PreInstallScriptStateExecutor &PreInstallScriptStateExecutor::instance()
{
    static PreInstallScriptStateExecutor inst(PREPARING);
    return inst;
}

#endif // ENVIRONMENTBUILDINGSTATEEXECUTOR_H
