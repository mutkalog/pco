#ifndef MANIFESTSERVICE_H
#define MANIFESTSERVICE_H

// #include "database.h"
#include "connectionspull.h"
#include <nlohmann/json.hpp>



using json = nlohmann::ordered_json;

class ManifestService
{
public:
    // ManifestService() : conn_(Database::instance().getConnection()) {}
    // ManifestService()
    //     : cp_(DB_CONNECTIONS_COUNT)
    // {}

    json getManifest(uint64_t devId, const std::string &devType,
                     const std::string &platform, const std::string &arch);
private:
    // std::unique_ptr<pqxx::connection> conn_;
    ConnectionsPool cp_;
};

#endif // MANIFESTSERVICE_H

