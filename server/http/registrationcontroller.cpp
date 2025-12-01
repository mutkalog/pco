#include "registrationcontroller.h"
#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;



void RegistrationController::registerRoute(httplib::Server &serv)
{
    serv.Post("/register", [&](const httplib::Request &req, httplib::Response& res) {
        std::cout << req.method << std::endl;
        std::cout << req.path << std::endl;

        std::cout << req.body << std::endl;

        json jsonbody;

        jsonbody["status"] = "registered";
        jsonbody["id"]     = 231;

        res.status = 200;
        std::string body = jsonbody.dump();
        res.set_content(body, "application/json");

        res.status = 200;
    });


}
