#ifndef ARTIFACTMANIFEST_H
#define ARTIFACTMANIFEST_H

#include <httplib.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include <bits/types/struct_tm.h>


struct ArtifactManifest
{
    struct {
        std::string version;
        std::string device;
        std::string platform;
        std::string arch;
        tm timestamp;
    } release;

    struct File {
        bool isExecutable;
        std::string installPath;
        struct {
            std::string algo;
            std::vector<uint8_t> value;
        } hash;
    };

    struct {
        std::string algo;
        std::string keyName;
        std::string base64value;
    } signature;

    std::vector<File> files;
    std::vector<std::string> requiredSharedLibraries;

public:
    void loadFromJson(const nlohmann::json &data);
    nlohmann::json saveInJson() const;

private:
    std::vector<uint8_t> rawHashFromString(const std::string& stringHash);
    std::string stringHashFromRaw(const std::vector<uint8_t>& rawHash) const;
};




#endif // ARTIFACTMANIFEST_H
