#ifndef UPLOADSERVICE_H
#define UPLOADSERVICE_H

#include <string>
#include <vector>
#include <filesystem>
#include <optional>

#include "database.h"
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
    UploadService() : conn_(Database::instance().getConnection()) {}
    void upload(ServerContext *sc, std::optional<int> canaryPercentage, const std::string& manifest, const std::string& archive);

private:
    void parseManifest(const std::string &raw, ReleasesTableEntry &entry);
    void parseFiles(const std::string &raw, ReleasesTableEntry &entry, const fs::path &bufDir, fs::path &storagePath);
    void commit(ServerContext *sc, std::optional<int> canaryPercentage, ReleasesTableEntry &entry, const fs::path &bufDir);
    std::unique_ptr<pqxx::connection> conn_;
};


#endif // UPLOADSERVICE_H
