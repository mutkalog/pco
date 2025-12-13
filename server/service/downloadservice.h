#ifndef DOWNLOADSERVICE_H
#define DOWNLOADSERVICE_H

#include "connectionspull.h"
#include "servercontext.h"
#include <cstdint>
#include <string>
#include <vector>


class DownloadService
{
public:
    std::vector<uint8_t> getArchive(std::shared_ptr<ServerContext> &sc, uint32_t devId, const std::string &devType,
                                    const std::string &platform, const std::string &arch);
private:
    ConnectionsPool cp_;
};

#endif // DOWNLOADSERVICE_H
