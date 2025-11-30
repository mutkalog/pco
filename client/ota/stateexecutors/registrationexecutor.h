#ifndef REGISTRATIONEXECUTOR_H
#define REGISTRATIONEXECUTOR_H

#include "stateexecutor.h"
#include "../statemachine.h"

class RegistrationExecutor final : public StateExecutor
{
public:
    static RegistrationExecutor& instance();
    virtual void execute(StateMachine& sm) override;

private:
    RegistrationExecutor(enum StateId id) : StateExecutor(id) {}
    void registerDevice(UpdateContext& ctx);
};

inline RegistrationExecutor &RegistrationExecutor::instance()
{
    static RegistrationExecutor inst(REGISTRATION);
    return inst;
}


#endif // REGISTRATIONEXECUTOR_H
