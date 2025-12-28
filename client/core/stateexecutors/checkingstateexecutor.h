#ifndef CHECKINGSTATEEXECUTOR_H
#define CHECKINGSTATEEXECUTOR_H

#include "core/artifactmanifest.h"
#include "core/interfaces/iclientconfig.h"
#include "core/stateexecutors/stateexecutor.h"
#include <nlohmann/json.hpp>

class UpdateContext;
using json = nlohmann::ordered_json;

class DeviceConfig;

class CheckingStateExecutor : public StateExecutor
{
public:
    virtual void execute(StateMachine& sm) override;
    CheckingStateExecutor(enum StateId id) : StateExecutor(id) {}

protected:
    virtual void process(StateMachine& sm, const std::string &responseBody);
    bool verificateRelease(const ArtifactManifest& received, const IClientConfig* current) const;
    bool compareVersions(const std::string& received, const std::string& current) const;
    bool compareDeviceType(const ArtifactManifest &received, const ArtifactManifest &current) const;
};

#endif // CHECKINGSTATEEXECUTOR_H
