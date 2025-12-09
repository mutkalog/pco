#ifndef FINALIZINGSTATEEXECUTOR_H
#define FINALIZINGSTATEEXECUTOR_H

#include "../statemachine.h"
#include "stateexecutor.h"

class FinalizingStateExecutor final : public StateExecutor
{
public:
    static FinalizingStateExecutor& instance();
    virtual void execute(StateMachine& sm) override;

    void rollback(UpdateContext &ctx);
    void launchScript(const fs::path &scriptPath);
    void totalCleanup(UpdateContext &ctx);
    void saveManifest(UpdateContext &ctx);

public:
    FinalizingStateExecutor(enum StateId id) : StateExecutor(id) {}
};

inline FinalizingStateExecutor &FinalizingStateExecutor::instance()
{
    static FinalizingStateExecutor inst(FINALIZING);
    return inst;
}


#endif // FINALIZINGSTATEEXECUTOR_H
