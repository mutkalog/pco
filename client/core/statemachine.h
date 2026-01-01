#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include "core/updatecontext.h"
#include "core/interfaces/istateexecutor.h"
#include "core/interfaces/istatepersistance.h"


class StateMachine
{
public:
    uint32_t state() { return currentSE_->id(); }

    void init();
    void run() { currentSE_->execute(*this); }
    void transitTo(uint32_t nextState);

    UpdateContext context;

    StateMachine(std::unique_ptr<IStatePersistence> sp,
                 UpdateContext ctx,
                 std::unordered_map<uint32_t, std::unique_ptr<IStateExecutor>> &&map,
                 uint32_t firstStateId,
                 uint32_t criticalStateId);

protected:
    void recover();

    bool inCriticalStates_;
    IStateExecutor* currentSE_;
    std::unique_ptr<IStatePersistence> sp_;
    const std::unordered_map<uint32_t, std::unique_ptr<IStateExecutor>> idToStateMap_;
    uint32_t criticalStateId_;
};




#endif // STATEMACHINE_H
