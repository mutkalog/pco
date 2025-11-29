#ifndef IDLESTATEEXECUTOR_H
#define IDLESTATEEXECUTOR_H

#include "stateexecutor.h"
#include "../statemachine.h"

class IdleStateExecutor final : public StateExecutor
{
public:
    static IdleStateExecutor& instance();

    virtual void execute(StateMachine& sm) override;

private:
    IdleStateExecutor(enum StateId id) : StateExecutor(id) {}

    void watchProcesses(UpdateContext &ctx, int gracefullExitMillis);
};

inline IdleStateExecutor &IdleStateExecutor::instance() {
    static IdleStateExecutor inst(IDLE);
    return inst;
}



#endif // IDLESTATEEXECUTOR_H
