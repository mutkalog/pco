#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include "updatecontext.h"
#include "stateexecutors/stateexecutor.h"
#include <optional>


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
    void recover();

    void dumpMachineState(StateExecutor::StateId state);
    std::optional<StateExecutor::StateId> loadMachineState();

    bool inCriticalStates_;
    StateExecutor *currentSE_;

    static StateExecutor* stateTable_[];
};



#endif // STATEMACHINE_H
