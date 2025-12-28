#ifndef MANIFESTSERVICE_H
#define MANIFESTSERVICE_H

#include "core/connectionspull.h"
#include "core/service/interfaces/imanifestservice.h"


class ManifestService : public IManifestService
{
    friend class ManifestServiceTest;

public:
    json getManifest(uint64_t devId, const std::string &devType,
                     const std::string &platform, const std::string &arch) override;

private:
    ConnectionsPool cp_;
};

#endif // MANIFESTSERVICE_H

