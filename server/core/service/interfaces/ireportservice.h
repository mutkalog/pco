#ifndef IREPORTSERVICE_H
#define IREPORTSERVICE_H

#include "core/servercontext.h"
#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;

class IReportService
{
public:
    virtual ~IReportService() = default;
    virtual void parseReport(std::shared_ptr<ServerContext> &sc, const json& report) = 0;
};

#endif // IREPORTSERVICE_H
