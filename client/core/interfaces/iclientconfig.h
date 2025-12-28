#ifndef ICLIENTCONFIG_H
#define ICLIENTCONFIG_H

#include "interfaces/iconfig.h"
#include "core/artifactmanifest.h"

class IClientConfig : public IConfig
{
public:
    virtual ~IClientConfig() = default;

    virtual void loadConfig() = 0;
    virtual void loadPrevManifest() = 0;
    virtual void saveNewUpdateInfo(const ArtifactManifest &newManifest) = 0;
    virtual void saveId(uint32_t id) = 0;

    virtual std::string      type()                   const = 0;
    virtual std::string      platform()               const = 0;
    virtual std::string      arch()                   const = 0;
    virtual int              pollingIntervalMinutes() const = 0;
    virtual std::string      serverUrl()              const = 0;
    virtual int              serverPort()             const = 0;
    virtual fs::path         publicKeyPath()          const = 0;
    virtual uint64_t         id()                     const = 0;
    virtual ArtifactManifest prevManifest()           const = 0;
};

#endif // ICLIENTCONFIG_H
