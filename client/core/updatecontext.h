#ifndef UPDATECONTEXT_H
#define UPDATECONTEXT_H

#include "core/interfaces/ihttpclient.h"
#include "core/artifactmanifest.h"
#include "core/deviceconfig.h"
#include "core/interfaces/isystemcalls.h"

#include "interfaces/icryptoutils.h"
#include "interfaces/iarchivetools.h"

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
    std::unique_ptr<IClientConfig> devconf;
    std::unique_ptr<IHttpClient> client;
    std::unique_ptr<ISSLUtils> cryptoUtils;
    std::unique_ptr<IArchiveTools> archiveTools;
    std::unique_ptr<ISystemCalls> syscalls;
    std::pair<int, std::string> reportMessage;
    std::unordered_map<fs::path, fs::path> pathToRollbackPathMap;
    BusyResources busyResources;
    bool recovering;

    UpdateContext(std::unique_ptr<IClientConfig> dc,
                  std::unique_ptr<IHttpClient> client,
                  std::unique_ptr<ISSLUtils> cryptoUtils,
                  std::unique_ptr<IArchiveTools> archiveTools,
                  std::unique_ptr<ISystemCalls> syscalls,
                  fs::path stagingDir, fs::path lastUpdateFile);

    void updateEnvironmentVars();
    json dumpContext() const;
    void loadContext(const json& ctx);
};

#endif // UPDATECONTEXT_H
