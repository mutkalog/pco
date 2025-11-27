#include "checkingstateexecutor.h"
#include "../../utils/utils.h"

#include "dowloadstateexecutor.h"
#include "idlestateexecutor.h"
#include "../statemachine.h"

void CheckingStateExecutor::execute(StateMachine &sm)
{
    auto& ctx = sm.context;

    auto res  = ctx.client.Get("/manifest?type=rpi4&place=machine&id=72");
    json data = json::parse(res->body);

    std::string rawManifest        = data["manifest"].get<std::string>();

    json signatureInfo             = json::parse(data["signature"].get<std::string>());
    std::string encodedSignature   = signatureInfo["signature"]["value"].get<std::string>();
    std::vector<uint8_t> signature = SSLUtils::decodeBase64(encodedSignature);

    if (SSLUtils::verifySignature(
            rawManifest, signature,
            std::string(PROJECT_ROOT_DIR) + "/keys/public.pem") == false)
    {
        throw std::runtime_error("Signature has not been verified");

        sm.transitTo(&IdleStateExecutor::instance());
        exit(0);
    }

    ctx.manifest.loadFromJson(json::parse(rawManifest));
    sm.transitTo(&DowloadStateExecutor::instance());
}

// void CheckingStateExecutor::loadManifestFromJson(ArtifactManifest& manifest, const nlohmann::json &data)
// {
//     manifest.release.version  = data["release"]["version"] .get<std::string>();
//     manifest.release.device   = data["release"]["type"]    .get<std::string>();
//     manifest.release.platform = data["release"]["platform"].get<std::string>();
//     manifest.release.arch     = data["release"]["arch"]    .get<std::string>();
//     std::istringstream ss(data["release"]["timestamp"]     .get<std::string>());

//     ss >> std::get_time(&manifest.release.timestamp, "%Y-%m-%dT%H:%M:%SZ");

//     for (const auto& file : data["files"])
//     {
//         ArtifactManifest::File entry;
//         entry.isExecutable = file["executable"]   .get<bool>();
//         entry.installPath  = file["path"]         .get<std::string>();
//         entry.hash.algo    = file["hash"]["algo"] .get<std::string>();
//         entry.hash.value   = parseHashFromString(file["hash"]["value"].get<std::string>());

//         manifest.files.push_back(std::move(entry));
//     }

//     // manifest.signature.algo     = data["signature"]["algo"].get<std::string>();
//     // manifest.signature.keyName  = data["signature"]["keyname"].get<std::string>();
//     // manifest.signature.base64value = data["signature"]["value"].get<std::string>();

//     for (const auto& lib : data["requiredLibs"])
//     {
//         manifest.requiredSharedLibraries.push_back(lib.get<std::string>());
//     }

// }

// std::vector<uint8_t> CheckingStateExecutor::parseHashFromString(const std::string &stringHash)
// {
//     std::vector<uint8_t> hash;
//     hash.reserve(stringHash.size() / 2);

//     for (size_t i = 0; i != stringHash.size(); i += 2)
//     {
//         uint8_t byte = std::stoi(stringHash.substr(i, 2), nullptr, 16);
//         hash.push_back(static_cast<uint8_t>(byte));
//     }


//     return hash;
// }
