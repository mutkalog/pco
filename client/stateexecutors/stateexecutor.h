#ifndef STATEEXECUTOR_H
#define STATEEXECUTOR_H

#include <cstddef>

class StateMachine;

class StateExecutor
{
public:

    enum StateId : size_t
    {
        IDLE,
        CHECKING,
        DOWNLOADING,
        VERIFYING,
        BUILDING_ENVIRONMENT,
        TESTING,
        COMMITING,
        TOTAL = 7
    };

    StateExecutor(enum StateId id) : id_(id) {}

    virtual ~StateExecutor() = default;
    virtual void execute(StateMachine& sm) = 0;

    enum StateId id() { return id_; }

protected:
    const enum StateId id_;
};

#endif // STATEEXECUTOR_H
