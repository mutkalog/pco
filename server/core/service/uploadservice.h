#ifndef UPLOADSERVICE_H
#define UPLOADSERVICE_H

#include <string>
#include <vector>
#include <filesystem>
#include <optional>

#include "core/connectionspull.h"
#include "core/servercontext.h"
#include "core/service/interfaces/iuploadservice.h"


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

class UploadService : public IUploadService
{
    friend class UploadServiceTest;

public:
    UploadService(const fs::path& bufferdir, const fs::path storagedir);

    void upload(std::shared_ptr<ServerContext> &sc,
                std::optional<int> canaryPercentage,
                int requiredTimeMinutes,
                const std::string &manifest,
                const std::string &archive) override;

private:
    void parseManifest(const std::string &raw, ReleasesTableEntry &entry);
    void parseFiles(std::shared_ptr<ServerContext> &sc, const std::string &raw, ReleasesTableEntry &entry, const fs::path &bufDir, fs::path &storagePath);
    void commit(std::shared_ptr<ServerContext> &sc, std::optional<int> canaryPercentage, int requiredTimeMinutes, ReleasesTableEntry &entry, const fs::path &bufDir);

    ConnectionsPool cp_;
    const fs::path bufferDir_;
    const fs::path storageDir_;
};



#endif // UPLOADSERVICE_H
