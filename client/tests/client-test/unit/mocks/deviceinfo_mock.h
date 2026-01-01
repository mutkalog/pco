#ifndef DEVICEINFO_MOCK_H
#define DEVICEINFO_MOCK_H

#include <gmock/gmock.h>
#include "core/interfaces/iclientconfig.h"
#include "core/artifactmanifest.h"

class MockClientConfig : public IClientConfig
{
public:
    MOCK_METHOD(void, loadConfig, (), (override));
    MOCK_METHOD(void, loadPrevManifest, (), (override));
    MOCK_METHOD(void, saveNewUpdateInfo, (const ArtifactManifest&), (override));
    MOCK_METHOD(void, saveId, (uint32_t), (override));

    MOCK_METHOD(std::string, type, (), (const, override));
    MOCK_METHOD(std::string, platform, (), (const, override));
    MOCK_METHOD(std::string, arch, (), (const, override));
    MOCK_METHOD(int, pollingIntervalMinutes, (), (const, override));
    MOCK_METHOD(std::string, serverUrl, (), (const, override));
    MOCK_METHOD(int, serverPort, (), (const, override));
    MOCK_METHOD(fs::path, publicKeyPath, (), (const, override));
    MOCK_METHOD(uint64_t, id, (), (const, override));
    MOCK_METHOD(ArtifactManifest, prevManifest, (), (const, override));
    MOCK_METHOD(fs::path, certPath, (), (const, override));
    MOCK_METHOD(fs::path, keyPath, (), (const, override));
    MOCK_METHOD(fs::path, caCertPath, (), (const, override));
};


#endif // DEVICEINFO_MOCK_H
