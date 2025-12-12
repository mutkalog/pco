#ifndef REGISTRATIONEXECUTOR_H
#define REGISTRATIONEXECUTOR_H

#include "stateexecutor.h"
#include "../statemachine.h"

class RegistrationStateExecutor final : public StateExecutor
{
public:
    static RegistrationStateExecutor& instance();
    virtual void execute(StateMachine& sm) override;

private:
    RegistrationStateExecutor(enum StateId id) : StateExecutor(id) {}
    void registerDevice(UpdateContext& ctx);
};

inline RegistrationStateExecutor &RegistrationStateExecutor::instance()
{
    static RegistrationStateExecutor inst(REGISTRATION);
    return inst;
}


#endif // REGISTRATIONEXECUTOR_H
