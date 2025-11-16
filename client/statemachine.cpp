#include "statemachine.h"

#include "stateexecutors/idlestateexecutor.h"

StateMachine::StateMachine()
    : context("http://localhost:8080"),
    currentSE_(&IdleStateExecutor::instance())
{}
