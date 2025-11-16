#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include "data.h"
#include "stateexecutors/stateexecutor.h"

class StateMachine
{
public:
    static StateMachine& instance() {
        static StateMachine sm;
        return sm;
    }

    enum StateExecutor::StateId state() { return currentSE_->id(); }

    void transitTo(StateExecutor *se) { currentSE_ = se; }

    void run() { currentSE_->execute(*this); }

    UpdateContext context;

private:
    StateMachine();

    StateExecutor *currentSE_;
};

#endif // STATEMACHINE_H
