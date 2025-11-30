#include "statemachine.h"

#include "stateexecutors/registrationexecutor.h"

StateMachine::StateMachine()
    : currentSE_(&RegistrationExecutor::instance())
{}
