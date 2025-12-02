#include "reportcontroller.h"
#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;




void ReportController::registerRoute(httplib::Server &serv)
{
    serv.Post("/report", [&](const httplib::Request &req, httplib::Response& res) {

        json requestBody     = json::parse(req.body);

        std::string type     = requestBody.value("type", "");
        std::string arch     = requestBody.value("arch", "");
        std::string platform = requestBody.value("platform", "");

        std::cout << req.method << " on " << req.path << " from " << arch << " "
                  << type << " on " << platform << std::endl;

        service_.parseReport(requestBody);

        res.status = httplib::OK_200;
    });

    //// если приходит ошибка, то делаем предыдущую версию active

}
