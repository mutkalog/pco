#ifndef DATA_H
#define DATA_H

#include <string>
#include <vector>

#include <bits/types/struct_tm.h>

struct ArtifactInfo {
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
            std::string value;
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

#endif // DATA_H
