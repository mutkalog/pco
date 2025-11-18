#ifndef DATA_H
#define DATA_H

#include <httplib.h>
#include <string>
#include <vector>

#include <bits/types/struct_tm.h>

struct ArtifactManifest {
    struct {
        std::string version;
        std::string device;
        std::string platform;
        std::string arch;
        tm timestamp;
    } release;

    struct File {
        std::string installPath;
        struct {
            std::string algo;
            std::vector<uint8_t> value;
        } hash;
        bool isExecutable;
    };

    struct {
        std::string algo;
        std::string keyName;
        std::string base64value;
    } signature;

    std::vector<File> files;
    std::vector<std::string> requiredSharedLibraries;
};

struct UpdateContext
{
    httplib::Client client;
    ArtifactManifest manifest;
    std::string testingDir;
    bool signatureOk;
    bool hashsOk;

    UpdateContext(std::string httpClientSettings)
        : client{httpClientSettings}
        , manifest{}
        , testingDir{std::string("/tmp/quarantine")}
        , signatureOk{}
        , hashsOk{}
    {}
};

#endif // DATA_H
