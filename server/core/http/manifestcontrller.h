#ifndef MANIFESTCONTRLLER_H
#define MANIFESTCONTRLLER_H

#include "controller.h"
#include "core/service/interfaces/imanifestservice.h"


class ManifestController final : public Controller
{
public:
    ManifestController(std::shared_ptr<ServerContext>& sc, std::unique_ptr<IManifestService> service)
        : Controller(sc), service_(std::move(service)) {}
    virtual void registerRoute(httplib::Server& serv) override;

private:
    std::unique_ptr<IManifestService> service_;
};

#endif // MANIFESTCONTRLLER_H
