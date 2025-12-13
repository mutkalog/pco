#ifndef UPLOADCONTROLLER_H
#define UPLOADCONTROLLER_H

#include "controller.h"
#include "../service/uploadservice.h"


class UploadController final : public Controller
{
public:
    UploadController(std::shared_ptr<ServerContext>& sc) : Controller(sc) {}
    virtual void registerRoute(httplib::Server& serv) override;

private:
    UploadService service_;
};

#endif // UPLOADCONTROLLER_H
