#include "statemachine.h"

#include "stateexecutors/idlestateexecutor.h"

StateMachine::StateMachine()
    : currentSE_(&IdleStateExecutor::instance())
{}
