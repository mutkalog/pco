#ifndef UPLOADCONTROLLER_H
#define UPLOADCONTROLLER_H

#include "core/http/controller.h"
#include "core/service/interfaces/iuploadservice.h"


class UploadController final : public Controller
{
public:
    UploadController(std::shared_ptr<ServerContext> &sc, std::unique_ptr<IUploadService> service)
        : Controller(sc), service_(std::move(service)) {}
    virtual void registerRoute(httplib::Server &serv) override;

private:
    std::unique_ptr<IUploadService> service_;
};

#endif // UPLOADCONTROLLER_H
