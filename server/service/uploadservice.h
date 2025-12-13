#ifndef UPLOADSERVICE_H
#define UPLOADSERVICE_H

#include <string>
#include <vector>
#include <filesystem>
#include <optional>
#include "connectionspull.h"

#include "servercontext.h"


namespace fs = std::filesystem;

struct ReleasesTableEntry
{
    std::string manifest;
    std::string signature;
    std::string version;
    std::string type;
    std::string platform;
    std::string arch;
    std::vector<fs::path> storagePaths;
    std::vector<std::pair<fs::path, fs::path>> bufferPathToStoragePath;
};

class UploadService
{
public:
    void upload(std::shared_ptr<ServerContext> &sc,
                std::optional<int> canaryPercentage,
                int requiredTimeMinutes,
                const std::string &manifest,
                const std::string &archive);

private:
    void parseManifest(const std::string &raw, ReleasesTableEntry &entry);
    void parseFiles(const std::string &raw, ReleasesTableEntry &entry, const fs::path &bufDir, fs::path &storagePath);
    void commit(std::shared_ptr<ServerContext> &sc, std::optional<int> canaryPercentage, int requiredTimeMinutes, ReleasesTableEntry &entry, const fs::path &bufDir);
    ConnectionsPool cp_;
};


#endif // UPLOADSERVICE_H
