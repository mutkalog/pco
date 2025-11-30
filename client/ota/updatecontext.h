#ifndef UPDATECONTEXT_H
#define UPDATECONTEXT_H

#include <httplib.h>
#include <openssl/ssl.h>
#include <vector>

#include "processmanager.h"
#include "../../utils/messagequeue.h"
#include "artifactmanifest.h"
#include "sandboxes/sandbox.h"
#include "deviceinfo.h"

class LinuxSandboxInspector;

enum UpdateCode : size_t
{
    OK,
    HASHES_NOT_EQUAL,
    TEST_FAILED,
    INTERNAL_UPDATE_ERROR,
};

struct BusyResources
{
    uint16_t testingDirCreated : 1;
    uint16_t sandbox           : 1;
    uint16_t sandboxInspector  : 1;
    uint16_t reserved          : 13;
};

struct UpdateContext
{
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
    std::pair<int, std::string> reportMessage;
    BusyResources busyResources;

    UpdateContext();
};

#endif // UPDATECONTEXT_H
