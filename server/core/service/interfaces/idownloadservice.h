#ifndef IDOWNLOADSERVICE_H
#define IDOWNLOADSERVICE_H

#include "core/servercontext.h"

class IDownloadService
{
public:
    virtual ~IDownloadService() = default;
    virtual std::vector<uint8_t> getArchive(std::shared_ptr<ServerContext> &sc, uint32_t devId, const std::string &devType,
                                    const std::string &platform, const std::string &arch) = 0;
};

#endif // IDOWNLOADSERVICE_H
