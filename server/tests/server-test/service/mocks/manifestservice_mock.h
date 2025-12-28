#ifndef MANIFESTSERVICE_MOCK_H
#define MANIFESTSERVICE_MOCK_H

#include <gmock/gmock.h>
#include "core/service/interfaces/imanifestservice.h"

class MockManifestService : public IManifestService
{
public:
    MOCK_METHOD(json,
                getManifest,
                (uint64_t devId, const std::string &devType,
                const std::string &platform, const std::string &arch),
                (override));
};

#endif // MANIFESTSERVICE_MOCK_H
