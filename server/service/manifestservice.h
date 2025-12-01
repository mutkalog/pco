#ifndef MANIFESTSERVICE_H
#define MANIFESTSERVICE_H

#include <nlohmann/json.hpp>
#include <filesystem>


using json = nlohmann::ordered_json;
namespace fs = std::filesystem;
class ManifestService
{
public:
    ManifestService() = default;

    json getManifest(uint32_t devId, const std::string &devType,
                     const std::string &platform, const std::string &arch);
};


#endif // MANIFESTSERVICE_H

