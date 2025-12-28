#ifndef UPLOADSERVICE_MOCK_H
#define UPLOADSERVICE_MOCK_H

#include <gmock/gmock.h>
#include "core/service/interfaces/iuploadservice.h"

class MockUploadService : public IUploadService
{
public:
    MOCK_METHOD(void, upload,
                (std::shared_ptr<ServerContext> & sc,
                 std::optional<int> canaryPercentage, int requiredTimeMinutes,
                 const std::string &manifest, const std::string &archive),
                (override));
};

#endif // UPLOADSERVICE_MOCK_H
