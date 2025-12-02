#ifndef UPLOADSERVICE_H
#define UPLOADSERVICE_H

#include <string>
#include <vector>
#include <filesystem>



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
    UploadService() = default;
    void upload(const std::string& manifest, const std::string& archive);

private:
    ReleasesTableEntry entry_;
    void parseManifest(const std::string &raw);
    void parseFiles(const std::string &raw);
    void commit();
    void cleanupEntry();
};

inline void UploadService::cleanupEntry() {
    entry_.manifest.clear();
    entry_.signature.clear();
    entry_.version.clear();
    entry_.type.clear();
    entry_.platform.clear();
    entry_.arch.clear();
    entry_.storagePaths.clear();
    entry_.bufferPathToStoragePath.clear();
}

#endif // UPLOADSERVICE_H
