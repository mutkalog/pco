#ifndef MANIFESTSERVICE_H
#define MANIFESTSERVICE_H

// #include "database.h"
#include "connectionspull.h"
#include <nlohmann/json.hpp>



using json = nlohmann::ordered_json;

class ManifestService
{
public:
    json getManifest(uint64_t devId, const std::string &devType,
                     const std::string &platform, const std::string &arch);
private:
    ConnectionsPool cp_;
};

#endif // MANIFESTSERVICE_H

