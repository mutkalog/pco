#ifndef DOWNLOADCONTROLLER_H
#define DOWNLOADCONTROLLER_H

#include "core/http/controller.h"
#include "core/service/interfaces/idownloadservice.h"


class DownloadController final : public Controller
{
    friend class DownloadControllerTest;

public:
    DownloadController(std::shared_ptr<ServerContext>& sc, std::unique_ptr<IDownloadService> service)
        : Controller(sc), service_(std::move(service)) {}
    virtual void registerRoute(httplib::Server& serv) override;

private:
    std::unique_ptr<IDownloadService> service_;
};

#endif // DOWNLOADCONTROLLER_H
