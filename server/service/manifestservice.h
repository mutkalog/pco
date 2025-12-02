#ifndef MANIFESTSERVICE_H
#define MANIFESTSERVICE_H

#include <nlohmann/json.hpp>



using json = nlohmann::ordered_json;

class ManifestService
{
public:
    ManifestService() = default;

    json getManifest(uint64_t devId, const std::string &devType,
                     const std::string &platform, const std::string &arch);
};


#endif // MANIFESTSERVICE_H

