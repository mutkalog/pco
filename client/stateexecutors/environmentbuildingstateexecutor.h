#ifndef ENVIRONMENTBUILDINGSTATEEXECUTOR_H
#define ENVIRONMENTBUILDINGSTATEEXECUTOR_H

#include "../statemachine.h"
#include "stateexecutor.h"

class EnvironmentBuildingStateExecutor final : public StateExecutor
{
public:
    static EnvironmentBuildingStateExecutor& instance();
    virtual void execute(StateMachine& sm) override;

public:
    EnvironmentBuildingStateExecutor(enum StateId id) : StateExecutor(id) {}
};

inline EnvironmentBuildingStateExecutor &EnvironmentBuildingStateExecutor::instance()
{
    static EnvironmentBuildingStateExecutor inst(BUILDING_ENVIRONMENT);
    return inst;
}

#endif // ENVIRONMENTBUILDINGSTATEEXECUTOR_H
