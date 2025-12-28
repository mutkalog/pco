#ifndef IMANIFESTSERVICE_H
#define IMANIFESTSERVICE_H

#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;

class IManifestService
{
public:
    virtual ~IManifestService() = default;
    virtual json getManifest(uint64_t devId, const std::string &devType,
                     const std::string &platform, const std::string &arch) = 0;
};

#endif // IMANIFESTSERVICE_H
