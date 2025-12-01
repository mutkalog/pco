#ifndef REPORTCONTROLLER_H
#define REPORTCONTROLLER_H

#include "controller.h"


class ReportController final : public Controller
{
public:
    ReportController() : Controller() {}
    virtual void registerRoute(httplib::Server& serv) override;
};

#endif // REPORTCONTROLLER_H
