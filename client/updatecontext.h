#ifndef UPDATECONTEXT_H
#define UPDATECONTEXT_H

#include <httplib.h>
#include <string>
#include <vector>

#include "artifactmanifest.h"
#include "sandboxes/sandbox.h"
#include "deviceinfo.h"

class LinuxSandboxInspector;

struct UpdateContext
{
    bool signatureOk;
    bool hashsOk;
    bool finalDecision;
    fs::path testingDir;
    httplib::Client client;
    ArtifactManifest manifest;
    std::unique_ptr<Sandbox> sb;
    std::unique_ptr<SandboxInspector> sbi;
    std::unique_ptr<DeviceInfo> devinfo;
    std::vector<pid_t> containeredProcesees;

    UpdateContext(std::string httpClientSettings);
};

#endif // UPDATECONTEXT_H
