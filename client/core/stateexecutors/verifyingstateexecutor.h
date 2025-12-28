#ifndef VERIFYINGSTATEEXECUTOR_H
#define VERIFYINGSTATEEXECUTOR_H

#include "core/statemachine.h"

class VerifyingStateExecutor : public StateExecutor
{
public:
    virtual void execute(StateMachine& sm) override;
    VerifyingStateExecutor(enum StateId id) : StateExecutor(id) {}

protected:
    using hash_t = std::vector<uint8_t>;
    virtual void verifyHashes(const hash_t& lhs, const hash_t& rhs) const;
};

#endif // VERIFYINGSTATEEXECUTOR_H
