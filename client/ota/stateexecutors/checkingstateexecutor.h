#ifndef CHECKINGSTATEEXECUTOR_H
#define CHECKINGSTATEEXECUTOR_H

#include <httplib.h>
#include "../artifactmanifest.h"
#include "stateexecutor.h"
#include <nlohmann/json.hpp>

class UpdateContext;
using json = nlohmann::ordered_json;

class DeviceInfo;

class CheckingStateExecutor final : public StateExecutor
{
public:
    static CheckingStateExecutor& instance();

    virtual void execute(StateMachine& sm) override;

private:
    void process(StateMachine& sm, const std::string &responseBody);
    bool verificateRelease(const ArtifactManifest& received, const DeviceInfo* current) const;
    bool compareVersions(const std::string& received, const std::string& current) const;
    bool compareDeviceType(const ArtifactManifest &received, const ArtifactManifest &current) const;

    CheckingStateExecutor(enum StateId id) : StateExecutor(id) {}
};

inline CheckingStateExecutor &CheckingStateExecutor::instance()
{
    static CheckingStateExecutor inst(CHECKING);
    return inst;
}

#endif // CHECKINGSTATEEXECUTOR_H
