#ifndef DOWNLOADCONTROLLER_H
#define DOWNLOADCONTROLLER_H

#include "controller.h"
#include "service/downloadservice.h"


class DownloadController final : public Controller
{
public:
    DownloadController(ServerContext* sc) : Controller(sc) {}
    virtual void registerRoute(httplib::Server& serv) override;

private:
    DownloadService service_;
};

#endif // DOWNLOADCONTROLLER_H
