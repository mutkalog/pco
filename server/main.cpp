#include <httplib.h>
#include <nlohmann/json.hpp>
#include <fstream>


int main()
{
    using namespace httplib;
    Server server;
    using json = nlohmann::ordered_json;

    server.Get("/manifest", [](const Request& req, Response& res) {
        std::cout << req.method << std::endl;
        std::cout << req.path << std::endl;
        std::cout << req.body << std::endl;
        std::cout << req.get_param_value("id") << std::endl;
        std::cout << req.get_param_value("place") << std::endl;
        std::cout << req.get_param_value("type") << std::endl;

        json jsonbody;

        std::ifstream fr("/home/alexander/Projects/posix-compatable-ota/posix-compatable-ota/app1/manifest.json");
        if (fr.is_open() == false)
            throw std::runtime_error("cannot open manifest.json");

        std::string raw((std::istreambuf_iterator<char>(fr)), std::istreambuf_iterator<char>());

        std::ifstream fs("/home/alexander/Projects/posix-compatable-ota/posix-compatable-ota/app1/signature.json");
        if (fs.is_open() == false)
            throw std::runtime_error("cannot open manifest.json");

        std::string signatureInfo((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());

        jsonbody["manifest"] = raw;
        jsonbody["signature"] = signatureInfo;

        res.status = 200;
        std::string body = jsonbody.dump();
        res.set_content(body, "application/json");
    });

    server.listen("0.0.0.0", 8080);
}
