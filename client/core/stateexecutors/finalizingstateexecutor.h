#ifndef FINALIZINGSTATEEXECUTOR_H
#define FINALIZINGSTATEEXECUTOR_H

#include "core/statemachine.h"
#include "core/stateexecutors/stateexecutor.h"

class FinalizingStateExecutor : public StateExecutor
{
public:
    virtual void execute(StateMachine& sm) override;
    FinalizingStateExecutor(enum StateId id) : StateExecutor(id) {}

protected:
    virtual void rollback(UpdateContext &ctx);
    virtual void launchScript(UpdateContext &ctx, const fs::path &scriptPath);
    virtual void totalCleanup(UpdateContext &ctx);
    virtual void sleep(std::chrono::minutes m);
};

#endif // FINALIZINGSTATEEXECUTOR_H
