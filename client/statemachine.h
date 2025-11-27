#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include "updatecontext.h"
#include "stateexecutors/stateexecutor.h"

class StateMachine
{
public:
    static StateMachine& instance() {
        static StateMachine sm;
        return sm;
    }

    enum StateExecutor::StateId state() { return currentSE_->id(); }

    void transitTo(StateExecutor *se) {
        std::cout << "Transition to " << se->id() << std::endl;
        std::cout.flush();
        currentSE_ = se;
    }

    void run() { currentSE_->execute(*this); }

    UpdateContext context;

private:
    StateMachine();

    StateExecutor *currentSE_;
};

#endif // STATEMACHINE_H
