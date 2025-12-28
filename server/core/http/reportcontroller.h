#ifndef REPORTCONTROLLER_H
#define REPORTCONTROLLER_H

#include "core/http/controller.h"
#include "core/service/interfaces/ireportservice.h"


class ReportController final : public Controller
{
public:
    ReportController(std::shared_ptr<ServerContext>& sc, std::unique_ptr<IReportService> service)
        : Controller(sc), service_(std::move(service)) {}
    virtual void registerRoute(httplib::Server& serv) override;

private:
    std::unique_ptr<IReportService> service_;
};

#endif // REPORTCONTROLLER_H
