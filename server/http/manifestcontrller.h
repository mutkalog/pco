#ifndef MANIFESTCONTRLLER_H
#define MANIFESTCONTRLLER_H

#include "controller.h"
#include "../service/manifestservice.h"


class ManifestController final : public Controller
{
public:
    ManifestController() : Controller() {}
    virtual void registerRoute(httplib::Server& serv) override;

private:
    ManifestService service_;
};


#endif // MANIFESTCONTRLLER_H
