#ifndef MANIFESTCONTRLLER_H
#define MANIFESTCONTRLLER_H

#include "controller.h"
#include "../service/manifestservice.h"


class ManifestController final : public Controller
{
public:
    ManifestController(std::shared_ptr<ServerContext>& sc) : Controller(sc) {}
    virtual void registerRoute(httplib::Server& serv) override;

private:
    ManifestService service_;
};


#endif // MANIFESTCONTRLLER_H
