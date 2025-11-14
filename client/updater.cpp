#include "updater.h"
#include "archive.h"
#include "../utils/utils.h"
#include <fstream>

bool removeSignature(std::string& raw)
{
    int start, end;

    auto sigPos = raw.find("signature");
    if (sigPos == std::string::npos)
        return false;

    sigPos = raw.rfind(",", sigPos - 1);
    start = (raw[sigPos + 1] == '\n')
            ? sigPos + 1
            : sigPos;

    auto lastBracket = raw.rfind("}");
    auto preLastBracket = raw.rfind("}", lastBracket - 1);

    end = (raw[preLastBracket + 1] == '\n')
          ? preLastBracket + 1
          : preLastBracket;

    raw = raw.erase(start, end - start);

    std::fstream of("/home/alexander/Projects/posix-compatable-ota/posix-compatable-ota/app1/procJOSNO", std::ios_base::out);
    of.write(reinterpret_cast<const char*>(raw.data()), raw.size());

    return true;
}

void Updater::getManifest()
{
    auto res  = client.Get("/manifest?type=rpi4&place=machine&id=72");
    json data = json::parse(res->body);

    std::string rawManifest        = data["manifest"].get<std::string>();

    json signatureInfo             = json::parse(data["signature"].get<std::string>());
    std::string encodedSignature   = signatureInfo["signature"]["value"].get<std::string>();
    std::vector<uint8_t> signature = SSLUtils::decodeBase64(encodedSignature);


    if (SSLUtils::verifySignature(
            rawManifest, signature,
            "/home/alexander/Projects/posix-compatable-ota/posix-compatable-ota/keys/public.pem"))
    {
        std::cout << "SIGNATURE CORRECT!\n";
    }
    else
    {
        std::cout << "INVALID SIGNATURE!\n";
    }

    fillArtifact(json::parse(rawManifest));
}

Updater::Updater()
    : client("http://localhost:8080")
{}

void Updater::fillArtifact(const nlohmann::json& data)
{
    m_artifact.release.version  = data["release"]["version"] .get<std::string>();
    m_artifact.release.device   = data["release"]["type"]    .get<std::string>();
    m_artifact.release.platform = data["release"]["platform"].get<std::string>();
    m_artifact.release.arch     = data["release"]["arch"]    .get<std::string>();
    std::istringstream ss(data["release"]["timestamp"]       .get<std::string>());

    ss >> std::get_time(&m_artifact.release.timestamp, "%Y-%m-%dT%H:%M:%SZ");

    for (const auto& file : data["files"])
    {
        ArtifactInfo::File entry;
        entry.installPath = file["path"]         .get<std::string>();
        entry.hash.algo   = file["hash"]["algo"] .get<std::string>();
        entry.hash.value  = file["hash"]["value"].get<std::string>();

        m_artifact.files.push_back(std::move(entry));
    }

    // m_artifact.signature.algo     = data["signature"]["algo"].get<std::string>();
    // m_artifact.signature.keyName  = data["signature"]["keyname"].get<std::string>();
    // m_artifact.signature.base64value = data["signature"]["value"].get<std::string>();

    for (const auto& lib : data["requiredLibs"])
    {
        m_artifact.requiredSharedLibraries.push_back(lib.get<std::string>());
    }
}
