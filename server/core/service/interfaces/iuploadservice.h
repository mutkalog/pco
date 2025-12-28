#ifndef IUPLOADSERVICE_H
#define IUPLOADSERVICE_H

#include "core/servercontext.h"
#include <optional>


class IUploadService
{
public:
    virtual ~IUploadService() = default;

    virtual void upload(std::shared_ptr<ServerContext> &sc,
                std::optional<int> canaryPercentage,
                int requiredTimeMinutes,
                const std::string &manifest,
                const std::string &archive) = 0;
};

#endif // IUPLOADSERVICE_H
