#ifndef UPLOADSERVICE_H
#define UPLOADSERVICE_H

#include <string>
#include <vector>

struct ReleasesTableEntry
{
    std::string manifest;
    std::string signature;
    std::string version;
    std::string type;
    std::string platform;
    std::string arch;
    std::vector<std::string> paths;
};

class UploadService
{
public:
    UploadService() = default;
    void parseManifest(const std::string &raw);
    void parseFiles(const std::string &raw);
    void commit();

private:
    ReleasesTableEntry entry_;
    bool manifestParsed;
    bool filesParsed;
};

#endif // UPLOADSERVICE_H
