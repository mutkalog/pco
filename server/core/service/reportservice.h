#ifndef REPORTSERVICE_H
#define REPORTSERVICE_H

#include "core/connectionspull.h"
#include "core/service/interfaces/ireportservice.h"


class ReportService : public IReportService
{
    friend class ReportServiceTest;

public:
    void parseReport(std::shared_ptr<ServerContext> &sc, const json& report) override;

private:
    ConnectionsPool cp_;
};

#endif // REPORTSERVICE_H
