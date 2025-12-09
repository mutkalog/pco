#ifndef COMMITTINGSTATEEXECUTOR_H
#define COMMITTINGSTATEEXECUTOR_H

#include "../statemachine.h"


class CommittingStateExecutor final : public StateExecutor
{
public:
    static CommittingStateExecutor& instance();
    virtual void execute(StateMachine& sm) override;

private:
    CommittingStateExecutor(enum StateId id) : StateExecutor(id) {}
};

inline CommittingStateExecutor &CommittingStateExecutor::instance()
{
    static CommittingStateExecutor inst(COMMITTING);
    return inst;
}

#endif // COMMITTINGSTATEEXECUTOR_H
