#ifndef COMMITINGSTATEEXECUTOR_H
#define COMMITINGSTATEEXECUTOR_H

#include "stateexecutor.h"

class CommitingStateExecutor final : public StateExecutor
{
public:
    static CommitingStateExecutor& instance();
    virtual void execute(StateMachine& sm) override;

private:
    CommitingStateExecutor(enum StateId id) : StateExecutor(id) {}
};

inline CommitingStateExecutor &CommitingStateExecutor::instance()
{
    static CommitingStateExecutor inst(COMMITTING);
    return inst;
}

#endif // COMMITINGSTATEEXECUTOR_H
