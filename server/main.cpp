#include <httplib.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <unistd.h>


int main()
{
    using namespace httplib;
    Server server;
    using json = nlohmann::ordered_json;
    std::string testdir = std::string(PROJECT_ROOT_DIR) + "/app2";

    server.Get("/manifest", [&](const Request& req, Response& res) {
        std::cout << req.method << std::endl;
        std::cout << req.path << std::endl;
        std::cout << req.body << std::endl;
        // std::cout << req.get_param_value("id") << std::endl;
        // std::cout << req.get_param_value("place") << std::endl;
        // std::cout << req.get_param_value("type") << std::endl;

        json jsonbody;

        std::ifstream fr(testdir + "/manifest.json", std::ios_base::in | std::ios_base::binary);
        if (!fr) {
            throw std::runtime_error("cannot open manifest.json");
        }

        std::string raw((std::istreambuf_iterator<char>(fr)), std::istreambuf_iterator<char>());

        std::ifstream fs(testdir + "/signature.json");
        if (!fs) {
            throw std::runtime_error("cannot open signature.json");
        }

        std::string signatureInfo((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());

        jsonbody["manifest"] = raw;
        jsonbody["signature"] = signatureInfo;

        res.status = 200;
        std::string body = jsonbody.dump();
        res.set_content(body, "application/json");
    });


    server.Get("/download", [&](const Request& req, Response& res) {
        std::cout << req.method << std::endl;
        std::cout << req.path << std::endl;
        // std::cout << req.get_param_value("id") << std::endl;
        // std::cout << req.get_param_value("place") << std::endl;
        // std::cout << req.get_param_value("type") << std::endl;

        std::ifstream fArchive(testdir + "/app.tar.gz");
        if (!fArchive) {
            throw std::runtime_error("cannot open manifest.json");
        }

        std::vector<uint8_t> raw((std::istreambuf_iterator<char>(fArchive)), std::istreambuf_iterator<char>());

        res.status = 200;
        res.set_content(reinterpret_cast<const char*>(raw.data()), raw.size(), "application/gzip");
    });


    server.listen("0.0.0.0", 8080);
}
