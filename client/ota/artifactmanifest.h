#ifndef ARTIFACTMANIFEST_H
#define ARTIFACTMANIFEST_H

#include <httplib.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include <bits/types/struct_tm.h>
#include <filesystem>

namespace fs = std::filesystem;


struct ArtifactManifest
{
    struct {
        std::string version;
        std::string type;
        std::string platform;
        std::string arch;
        tm timestamp;
    } release;

    struct File {
        bool isExecutable;
        fs::path installPath;
        std::vector<std::string> args;
        struct {
            std::string algo;
            std::vector<uint8_t> value;
        } hash;
    };

    std::vector<File> files;

public:
    void clear();
    void loadFromJson(const nlohmann::json &data);
    nlohmann::json saveInJson() const;

    static std::vector<char *> getFileArgs(const File& file);

private:
    std::vector<uint8_t> rawHashFromString(const std::string& stringHash);
    std::string stringHashFromRaw(const std::vector<uint8_t>& rawHash) const;
};

inline void ArtifactManifest::clear()
{
    release.version.clear();
    release.type.clear();
    release.platform.clear();
    release.arch.clear();
    std::memset(&release.timestamp, 0, sizeof release.timestamp);
    release.timestamp = tm{};
    files.clear();
}

#endif // ARTIFACTMANIFEST_H
