#ifndef DATA_H
#define DATA_H

#include "sandboxes/linuxsandbox.h"
#include "sandboxes/linuxsandboxinspecor.h"
#include "sandboxes/sandbox.h"
#include <httplib.h>
#include <string>
#include <vector>

#include <bits/types/struct_tm.h>

class LinuxSandboxInspector;


struct ArtifactManifest {
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
};

struct UpdateContext
{
    httplib::Client client;
    ArtifactManifest manifest;
    std::string testingDir;
    bool signatureOk;
    bool hashsOk;
    std::vector<pid_t> containeredProcesees;
    std::unique_ptr<Sandbox> sb;
    std::unique_ptr<SandboxInspector> sbi;

    UpdateContext(std::string httpClientSettings);
};

inline UpdateContext::UpdateContext(std::string httpClientSettings)
    : client{httpClientSettings}
    , manifest{}
    , testingDir{std::string("/tmp/quarantine")}
    , signatureOk{}
    , hashsOk{}
#if defined(__linux__)
    , sb{std::make_unique<LinuxSandbox>()}
    , sbi{std::make_unique<LinuxSandboxInspector>()}
#endif
{
}

#endif // DATA_H
