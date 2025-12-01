#include "reportcontroller.h"


void ReportController::registerRoute(httplib::Server &serv)
{
    serv.Post("/report", [&](const httplib::Request &req, httplib::Response& res) {
        std::cout << req.method << std::endl;
        std::cout << req.path << std::endl;

        std::cout << req.body << std::endl;

        res.status = 200;
    });


}
