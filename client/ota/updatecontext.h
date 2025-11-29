#ifndef UPDATECONTEXT_H
#define UPDATECONTEXT_H

#include <httplib.h>
#include <openssl/ssl.h>
#include <vector>

#include "../processmanager.h"
#include "../../utils/messagequeue.h"
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
    ArtifactManifest manifest;
    std::unique_ptr<Sandbox> sb;
    std::unique_ptr<DeviceInfo> devinfo;
    std::unique_ptr<ProcessManager> pm;
    std::unique_ptr<SandboxInspector> sbi;
    std::unique_ptr<httplib::SSLClient> client;
    std::vector<pid_t> containeredProcesees;
    std::shared_ptr<MessageQueue<ChildInfo>> supervisorMq;

    UpdateContext();
};

#endif // UPDATECONTEXT_H
