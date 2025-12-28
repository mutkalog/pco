#ifndef DOWNLOADSERVICE_H
#define DOWNLOADSERVICE_H

#include "core/connectionspull.h"
#include "core/service/interfaces/idownloadservice.h"


class DownloadService : public IDownloadService
{
    friend class DownloadServiceTest;

public:
    std::vector<uint8_t> getArchive(std::shared_ptr<ServerContext> &sc, uint32_t devId, const std::string &devType,
                                    const std::string &platform, const std::string &arch) override;
private:
    ConnectionsPool cp_;
};

#endif // DOWNLOADSERVICE_H
