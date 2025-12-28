#ifndef IARTIFACTMANIFEST_H
#define IARTIFACTMANIFEST_H

#include <nlohmann/json.hpp>

class IArtifactManifest
{
public:
    virtual void clear() = 0;
    virtual void loadFromJson(const nlohmann::json &data) = 0;
    virtual nlohmann::json saveInJson() const = 0;
    virtual std::vector<uint8_t> rawHashFromString(const std::string& stringHash) = 0;
    virtual std::string stringHashFromRaw(const std::vector<uint8_t>& rawHash) const = 0;
};
#endif // IARTIFACTMANIFEST_H
