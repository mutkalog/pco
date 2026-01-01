#ifndef DOWLOADINGSTATEEXECUTOR_H
#define DOWLOADINGSTATEEXECUTOR_H

#include "core/stateexecutors/stateexecutor.h"
// #include <chrono>

class DownloadingStateExecutor : public StateExecutor
{
public:
    virtual void execute(StateMachine& sm) override;
    DownloadingStateExecutor(enum StateId id) : StateExecutor(id) {}

protected:
    virtual void process(StateMachine& sm, const std::string &responseBody);
    // virtual void sleep(std::chrono::minutes m);
};

#endif // DOWLOADINGSTATEEXECUTOR_H
