#include "registrationcontroller.h"
#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;



void RegistrationController::registerRoute(httplib::Server &serv)
{
    serv.Post("/register", [&](const httplib::Request &req, httplib::Response& res) {

        json requestBody = json::parse(req.body);
        std::string type     = requestBody["type"];
        std::string arch     = requestBody["arch"];
        std::string platform = requestBody["platform"];

        std::cout << req.method << " on " << req.path << " from " << arch << " "
                  << type << " on " << platform << std::endl;

        json jsonbody;
        try
        {
            uint64_t id        = service_.registerDevice(requestBody);
            jsonbody["status"] = "registered";
            jsonbody["id"]     = id;

            res.status = httplib::OK_200;
            std::string body = jsonbody.dump();
            res.set_content(body, "application/json");
        }
        catch (const std::system_error &ex)
        {
            throw;
        }
        catch (const std::runtime_error &ex)
        {
            std::cout << ex.what() << std::endl;

            res.status = httplib::BadRequest_400;
            std::string body = jsonbody.dump();
            res.set_content("Wrong parameters", "plain/text");
        }
    });
}
