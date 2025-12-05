#ifndef STATEEXECUTOR_H
#define STATEEXECUTOR_H

#include <cstddef>
#include <string>
#include <unordered_map>

class StateMachine;

class StateExecutor
{
public:

    enum StateId : size_t
    {
        REGISTRATION,
        IDLE,
        CHECKING,
        DOWNLOADING,
        VERIFYING,
        PREPARING,
        PRE_COMMITTING,
        COMMITTING,
        FINALIZING,
        TOTAL
    };

    StateExecutor(enum StateId id) : id_(id) {}

    virtual ~StateExecutor() = default;
    virtual void execute(StateMachine& sm) = 0;

    enum StateId id() const { return id_; }
    std::string textId() const { return idToNameMap_[id_]; }

protected:
    const enum StateId id_;

    static std::unordered_map<StateId, std::string> idToNameMap_;
};


#endif // STATEEXECUTOR_H
