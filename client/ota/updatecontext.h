#ifndef UPDATECONTEXT_H
#define UPDATECONTEXT_H

#include <httplib.h>
#include <openssl/ssl.h>

#include "artifactmanifest.h"
#include "deviceinfo.h"

class LinuxSandboxInspector;

enum UpdateCode : size_t
{
    OK,
    ARTIFACT_INTEGRITY_ERROR,
    TEST_FAILED,
    INTERNAL_UPDATE_ERROR,
};

struct BusyResources
{
    uint16_t testingDirCreated : 1;
    uint16_t reserved          : 15;
};

struct UpdateContext
{
    bool finalDecision;
    fs::path stagingDir;
    ArtifactManifest manifest;
    std::unique_ptr<DeviceInfo> devinfo;
    std::unique_ptr<httplib::SSLClient> client;
    std::pair<int, std::string> reportMessage;
    BusyResources busyResources;

    UpdateContext();
};

#endif // UPDATECONTEXT_H
