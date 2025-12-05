#ifndef DOWNLOADSERVICE_H
#define DOWNLOADSERVICE_H

#include "database.h"
#include "servercontext.h"
#include <cstdint>
#include <string>
#include <vector>



class DownloadService
{
public:
    DownloadService() : conn_(Database::instance().getConnection()) {}
    std::vector<uint8_t> getArchive(ServerContext *sc, uint32_t devId, const std::string &devType,
                                    const std::string &platform, const std::string &arch);
private:
    std::unique_ptr<pqxx::connection> conn_;
};

#endif // DOWNLOADSERVICE_H
