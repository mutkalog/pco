#ifndef DOWLOADSTATEEXECUTOR_H
#define DOWLOADSTATEEXECUTOR_H

#include "stateexecutor.h"

#include <ota/updatecontext.h>

class DowloadStateExecutor final : public StateExecutor
{
public:
    static DowloadStateExecutor& instance();
    virtual void execute(StateMachine& sm) override;

private:
    void process(StateMachine& sm, const std::string &responseBody);
    DowloadStateExecutor(enum StateId id) : StateExecutor(id) {}
};

inline DowloadStateExecutor &DowloadStateExecutor::instance()
{
    static DowloadStateExecutor inst(DOWNLOADING);
    return inst;
}

#endif // DOWLOADSTATEEXECUTOR_H
