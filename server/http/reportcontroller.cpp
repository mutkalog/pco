#include "reportcontroller.h"
#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;




void ReportController::registerRoute(httplib::Server &serv)
{
    serv.Post("/report", [&](const httplib::Request &req, httplib::Response& res) {

        json requestBody     = json::parse(req.body);

        uint64_t    devId    = requestBody.value("id", -1);
        std::string type     = requestBody.value("type", "");
        std::string arch     = requestBody.value("arch", "");
        std::string platform = requestBody.value("platform", "");

        std::cout << req.method << " on " << req.path << " from " << arch << " "
                  << type << " on " << platform << " with id " << devId << std::endl;

        service_.parseReport(sc_, requestBody);

        res.status = httplib::OK_200;
    });
}
