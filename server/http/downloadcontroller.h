#ifndef DOWNLOADCONTROLLER_H
#define DOWNLOADCONTROLLER_H

#include "controller.h"


class DownloadController final : public Controller
{
public:
    DownloadController() : Controller() {}
    virtual void registerRoute(httplib::Server& serv) override;
};

#endif // DOWNLOADCONTROLLER_H
