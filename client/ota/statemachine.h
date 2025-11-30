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

    void transitTo(StateExecutor *se);

    void run() { currentSE_->execute(*this); }

    UpdateContext context;

private:
    StateMachine();

    StateExecutor *currentSE_;
};

inline void StateMachine::transitTo(StateExecutor *se) {
    std::cout << "Transition to " << se->textId() << " state" << std::endl;
    currentSE_ = se;
}

#endif // STATEMACHINE_H
