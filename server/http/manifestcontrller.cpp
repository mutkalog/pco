#include "manifestcontrller.h"

#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;

void ManifestController::registerRoute(httplib::Server &serv)
{
    serv.Get("/manifest", [&](const httplib::Request &req, httplib::Response& res) {

        uint32_t    devId    = std::stoul(req.get_param_value("id"), nullptr, 10);
        std::string type     = req.get_param_value("type");
        std::string platform = req.get_param_value("platform");
        std::string arch     = req.get_param_value("arch");

        std::cout << req.method << " on " << req.path << " from " << arch << " "
                  << type << " on " << platform << std::endl;

        try
        {
            json jsonbody = service_.getManifest(devId, type, platform, arch);

            res.status = httplib::OK_200;
            std::string body = jsonbody.dump();
            res.set_content(body, "application/json");
        }
        catch (const std::exception& ex)
        {
            std::cout << ex.what() << std::endl;
            res.status = httplib::BadRequest_400;
            res.set_content("Release not found", "plain/text");
        }
    });
}
