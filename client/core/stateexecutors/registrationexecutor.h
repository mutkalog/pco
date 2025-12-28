#ifndef REGISTRATIONEXECUTOR_H
#define REGISTRATIONEXECUTOR_H

#include "stateexecutor.h"
#include "../statemachine.h"

class RegistrationStateExecutor final : public StateExecutor
{
public:
    virtual void execute(StateMachine& sm) override;

    RegistrationStateExecutor(enum StateId id) : StateExecutor(id) {}

private:
    void registerDevice(UpdateContext& ctx);
};

#endif // REGISTRATIONEXECUTOR_H
