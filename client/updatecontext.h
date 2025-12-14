#ifndef UPDATECONTEXT_H
#define UPDATECONTEXT_H

// #include <httplib.h>
// #include <openssl/ssl.h>
#include "sslhttpclient.h"

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
    uint32_t stagingDirCreated  : 1;
    uint32_t rollbacks          : 1;
    uint32_t reserved           : 29;
};

struct UpdateContext
{
    bool rollback;
    fs::path stagingDir;
    fs::path prevManifestPath;
    ArtifactManifest manifest;
    std::unique_ptr<DeviceInfo> devinfo;
    std::unique_ptr<IHttpClient> client;
    // std::unique_ptr<httplib::SSLClient> client;
    std::pair<int, std::string> reportMessage;
    std::unordered_map<fs::path, fs::path> pathToRollbackPathMap;
    BusyResources busyResources;
    bool recovering;

    UpdateContext();

    void updateEnvironmentVars();
    json dumpContext();
    void loadContext(const json& ctx);
};

#endif // UPDATECONTEXT_H
