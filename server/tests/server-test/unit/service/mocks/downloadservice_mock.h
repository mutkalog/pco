#ifndef DOWNLOADSERVICE_MOCK_H
#define DOWNLOADSERVICE_MOCK_H

#include <gmock/gmock.h>
#include "core/service/interfaces/idownloadservice.h"


class MockDownloadService : public IDownloadService
{
public:
    MOCK_METHOD(std::vector<uint8_t>,
                getArchive,
                (std::shared_ptr<ServerContext> & sc, uint32_t devId,
                  const std::string &devType, const std::string &platform,
                  const std::string &arch),
                (override));
};

#endif // DOWNLOADSERVICE_MOCK_H
