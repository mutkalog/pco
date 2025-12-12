#include "updatecontext.h"
#include "deviceinfo.h"

namespace {
const fs::path PCO_STAGING_DIR       = "/var/pco/staging";
const fs::path LAST_UPDATE_INFO_PATH = "/var/pco/last-update.json";
}


UpdateContext::UpdateContext()
    : manifest{}
    , stagingDir{PCO_STAGING_DIR}
    , prevManifestPath{LAST_UPDATE_INFO_PATH}
    , rollback{false}
    , reportMessage{}
    , busyResources{}
    , recovering{false}
{
    try
    {
        devinfo = std::make_unique<DeviceInfo>();

        std::string clientCert = devinfo->certPath().string();
        std::string clientKey  = devinfo->keyPath().string();
        std::string caCert     = devinfo->caCertPath().string();
        std::string host       = devinfo->serverUrl();
        int         port       = devinfo->serverPort();

        client = std::make_unique<httplib::SSLClient>(host, port, clientCert, clientKey);
        client->set_ca_cert_path(caCert);
        client->enable_server_certificate_verification(true);

        updateEnvironmentVars();

        if (recovering == true)
        {
            manifest = devinfo->prevManifest();
        }
    }
    catch (const nlohmann::detail::exception& e)
    {
        std::cout << e.what() << std::endl;
        std::cout << "UpdateContext: JSON parse error" << std::endl;
        if (devinfo->type().empty() == true
            || devinfo->serverUrl().empty() == true)
        {
            std::cout << "UpdateContext: Cannot continue without critical device info."
                         " Aborting..." << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    catch (const std::system_error& e)
    {
        std::cout << "UpdateContext: something went wrong."
                     " Aborting" << std::endl;
        std::cout << e.what() << " " << e.code().message() << std::endl;
        exit(e.code().value());
    }
    catch (const std::runtime_error& e)
    {
        std::cout << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
    catch (const std::exception& e)
    {
        std::cout << "UpdateContext: unknown exception -- "
                  << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
}

void UpdateContext::updateEnvironmentVars()
{
    std::string pcoRollbackArtifactsPaths;
    for (const auto& file : devinfo->prevManifest().files)
    {
        if (file.isScript == false)
        {
            pcoRollbackArtifactsPaths += file.installPath.string() + ":";
        }
    }

    pcoRollbackArtifactsPaths  = pcoRollbackArtifactsPaths.substr(0, pcoRollbackArtifactsPaths.size() - 1);

    std::cout << "PCO_ROLLBACK_ARTIFACTS_PATHS: "  << pcoRollbackArtifactsPaths  << std::endl;
    if (setenv("PCO_ROLLBACK_ARTIFACTS_PATHS", pcoRollbackArtifactsPaths.c_str(), 1) != 0)
    {
        throw std::system_error(std::error_code(errno, std::generic_category()),
                "Cannot setenv PCO_ROLLBACK_ARTIFACTS_PATHS");
    }
}

json UpdateContext::dumpContext()
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
