#include "updatecontext.h"
#include "deviceconfig.h"

UpdateContext::UpdateContext(std::unique_ptr<IClientConfig> dc,
                             std::unique_ptr<IHttpClient> c,
                             std::unique_ptr<ISSLUtils> cu,
                             std::unique_ptr<IArchiveTools> at,
                             std::unique_ptr<ISystemCalls> sc,
                             fs::path stagingDir, fs::path lastUpdateFile)
    : manifest{}
    , stagingDir{stagingDir}
    , prevManifestPath{lastUpdateFile}
    , devconf{std::move(dc)}
    , client{std::move(c)}
    , cryptoUtils{std::move(cu)}
    , archiveTools{std::move(at)}
    , syscalls{std::move(sc)}
    , rollback{false}
    , reportMessage{}
    , pathToRollbackPathMap{}
    , busyResources{}
    , recovering{false}
{}

void UpdateContext::updateEnvironmentVars()
{
    std::string pcoRollbackArtifactsPaths;
    for (const auto& file : devconf->prevManifest().files)
    {
        if (file.isScript == false)
        {
            pcoRollbackArtifactsPaths += file.installPath.string() + ":";
        }
    }

    pcoRollbackArtifactsPaths = pcoRollbackArtifactsPaths.substr(0, pcoRollbackArtifactsPaths.size() - 1);

    std::cout << "PCO_ROLLBACK_ARTIFACTS_PATHS: "  << pcoRollbackArtifactsPaths  << std::endl;
    if (setenv("PCO_ROLLBACK_ARTIFACTS_PATHS", pcoRollbackArtifactsPaths.c_str(), 1) != 0)
    {
        throw std::system_error(std::error_code(errno, std::generic_category()),
                "Cannot setenv PCO_ROLLBACK_ARTIFACTS_PATHS");
    }
}


json UpdateContext::dumpContext() const
{
    nlohmann::json j;

    j["rollback"]         = rollback;
    j["manifest"]         = manifest.saveInJson();
    j["reportMessage"]    = {reportMessage.first, reportMessage.second};
    j["recovering"]       = recovering;

    nlohmann::json mapJson;
    for (const auto& [k, v] : pathToRollbackPathMap)
    {
        mapJson[k.string()] = v.string();
    }

    j["pathToRollbackPathMap"] = mapJson;

    uint32_t word;
    std::memcpy(&word, &busyResources, sizeof(BusyResources));
    j["busyResources"] = word;

    return j;
}

void UpdateContext::loadContext(const json &ctx)
{
    rollback         = ctx["rollback"];
    reportMessage    = {ctx["reportMessage"][0].get<int>(),
                        ctx["reportMessage"][1].get<std::string>()};
    recovering       = ctx["recovering"];

    manifest.loadFromJson(ctx["manifest"]);

    for (auto it = ctx["pathToRollbackPathMap"].begin();
         it != ctx["pathToRollbackPathMap"].end();
         ++it)
    {
        pathToRollbackPathMap[it.key()] = it.value().get<std::string>();
    }

    uint32_t word = ctx["busyResources"];
    std::memcpy(&busyResources, &word, sizeof(uint32_t));
}
