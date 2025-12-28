#ifndef DEVICECONFIG_H
#define DEVICECONFIG_H

#include "interfaces/iclientconfig.h"
#include <nlohmann/json.hpp>


using json = nlohmann::ordered_json;


/// Класс, хранящий информацию об устройстве
class DeviceConfig : public IClientConfig
{
public:
    DeviceConfig(fs::path confPath, fs::path lastUpdateFile);

    virtual void loadConfig() override;
    virtual void loadPrevManifest() override;
    virtual void saveNewUpdateInfo(const ArtifactManifest &newManifest) override;
    virtual void saveId(uint32_t id) override;

    virtual std::string      type()                   const override { return type_; }
    virtual std::string      platform()               const override { return platform_; }
    virtual std::string      arch()                   const override { return arch_; }
    virtual int              pollingIntervalMinutes() const override { return pollingIntervalMinutes_; }
    virtual std::string      serverUrl()              const override { return serverUrl_; }
    virtual int              serverPort()             const override { return serverPort_; }
    virtual fs::path         certPath()               const override { return certPath_; }
    virtual fs::path         keyPath()                const override { return keyPath_; }
    virtual fs::path         caCertPath()             const override { return caCertPath_; }
    virtual fs::path         publicKeyPath()          const override { return publicKeyPath_; }
    virtual uint64_t         id()                     const override { return id_; }
    virtual ArtifactManifest prevManifest()           const override { return prevManifest_; }

private:
    std::string      type_;
    std::string      platform_;
    std::string      arch_;
    int              pollingIntervalMinutes_;
    std::string      serverUrl_;
    int              serverPort_;
    fs::path         certPath_;
    fs::path         keyPath_;
    fs::path         caCertPath_;
    fs::path         publicKeyPath_;
    uint32_t         id_;
    ArtifactManifest prevManifest_;

    const fs::path confFile_;
    const fs::path lastUpdateFile_;
};

#endif // DEVICECONFIG_H
