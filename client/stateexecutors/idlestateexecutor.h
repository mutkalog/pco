#ifndef IDLESTATEEXECUTOR_H
#define IDLESTATEEXECUTOR_H

#include "stateexecutor.h"
#include "../statemachine.h"
#include "checkingstateexecutor.h"


#include <chrono>
#include <thread>

namespace {
    int pollIntervalMillis = 0.5e3;
}

class IdleStateExecutor final : public StateExecutor
{
public:
    static IdleStateExecutor& instance();

    virtual void execute(StateMachine& sm) override;

private:
    IdleStateExecutor(enum StateId id) : StateExecutor(id) {}
};

inline IdleStateExecutor &IdleStateExecutor::instance() {
    static IdleStateExecutor inst(IDLE);
    return inst;
}

inline void IdleStateExecutor::execute(StateMachine &sm)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(pollIntervalMillis));
    sm.transitTo(&CheckingStateExecutor::instance());
}

#endif // IDLESTATEEXECUTOR_H
