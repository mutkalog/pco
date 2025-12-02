#ifndef REPORTCONTROLLER_H
#define REPORTCONTROLLER_H

#include "controller.h"
#include "service/reportservice.h"


class ReportController final : public Controller
{
public:
    ReportController() : Controller() {}
    virtual void registerRoute(httplib::Server& serv) override;

private:
    ReportService service_;
};

#endif // REPORTCONTROLLER_H
