#ifndef STATEEXECUTOR_H
#define STATEEXECUTOR_H

#include <cstdint>
#include <string>
#include <unordered_map>

#include "core/interfaces/istateexecutor.h"


class StateExecutor : public IStateExecutor
{
public:
    enum StateId : uint32_t
    {
        REGISTRATION,
        IDLE,
        CHECKING,
        DOWNLOADING,
        VERIFYING,
        PREPARING,
        INSTALLING,
        COMMITTING,
        FINALIZING,
        TOTAL
    };

    StateExecutor(enum StateId id) : id_(id) {}

    uint32_t id() const override { return id_; }
    std::string textId() const override { return idToNameMap_[id_]; }
    virtual void setEnvVar(const std::string& key, const std::string& value) const;

protected:
    const enum StateId id_;
    static std::unordered_map<StateId, std::string> idToNameMap_;
};


#endif // STATEEXECUTOR_H
