#include "artifactmanifest.h"

void ArtifactManifest::loadFromJson(const nlohmann::json &data)
{
    release.version  = data["release"]["version"] .get<std::string>();
    release.device   = data["release"]["type"]    .get<std::string>();
    release.platform = data["release"]["platform"].get<std::string>();
    release.arch     = data["release"]["arch"]    .get<std::string>();
    std::istringstream ss(data["release"]["timestamp"]     .get<std::string>());

    ss >> std::get_time(&release.timestamp, "%Y-%m-%dT%H:%M:%SZ");

    for (const auto& file : data["files"])
    {
        ArtifactManifest::File entry;
        entry.isExecutable = file["executable"]   .get<bool>();
        entry.installPath  = file["path"]         .get<std::string>();
        entry.hash.algo    = file["hash"]["algo"] .get<std::string>();
        entry.hash.value   = rawHashFromString(file["hash"]["value"].get<std::string>());

        files.push_back(std::move(entry));
    }

    // signature.algo     = data["signature"]["algo"].get<std::string>();
    // signature.keyName  = data["signature"]["keyname"].get<std::string>();
    // signature.base64value = data["signature"]["value"].get<std::string>();

    for (const auto& lib : data["requiredLibs"])
    {
        requiredSharedLibraries.push_back(lib.get<std::string>());
    }
}

nlohmann::json ArtifactManifest::saveInJson() const
{
    nlohmann::json data;

    std::array<char, 32> buf;
    strftime(buf.data(), buf.size(), "%Y-%m-%dT%H:%M:%SZ", &release.timestamp);

    data["release"] = {
        { "version",                      release.version },
        { "type",                         release.device  },
        { "platform",                     release.platform},
        { "arch",                         release.arch    },
        { "timestamp", std::string(buf.begin(), buf.end())}
    };

    data["files"] = nlohmann::json::array();

    for (const auto& file : files)
    {
        data["files"].push_back({
            { "executable", file.isExecutable },
            { "path",       file.installPath  },
            { "hash",
            {
                { "algo",  file.hash.algo                     },
                { "value", stringHashFromRaw(file.hash.value) }
            }
            }
        });
    }

    data["signature"] = {
        { "algo",    signature.algo        },
        { "keyname", signature.keyName     },
        { "value",   signature.base64value }
    };

    data["requiredLibs"] = nlohmann::json::array();
    for (const auto& l : requiredSharedLibraries)
        data["requiredLibs"].push_back(l);

    return data;
}

std::vector<uint8_t> ArtifactManifest::rawHashFromString(const std::string &stringHash)
{
    std::vector<uint8_t> hash;
    hash.reserve(stringHash.size() / 2);

    for (size_t i = 0; i != stringHash.size(); i += 2) {
        uint8_t byte = std::stoi(stringHash.substr(i, 2), nullptr, 16);
        hash.push_back(static_cast<uint8_t>(byte));
    }

    return hash;
}

std::string ArtifactManifest::stringHashFromRaw(const std::vector<uint8_t> &rawHash) const
{
    std::string hex;
    hex.reserve(rawHash.size() * 2);
    const char* digits = "0123456789abcdef";

    for (uint8_t byte : rawHash)
    {
        hex.push_back(digits[byte >> 4]);
        hex.push_back(digits[byte & 0x0F]);
    }

    return hex;
}
