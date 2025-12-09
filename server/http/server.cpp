#include "server.h"
#include <filesystem>

namespace fs = std::filesystem;

#include <nlohmann/json.hpp>
using json = nlohmann::ordered_json;

namespace {
    std::string ca_cert     = fs::path(PROJECT_ROOT_DIR) / "mtls/ca.pem";
    std::string server_cert = fs::path(PROJECT_ROOT_DIR) / "mtls/server/server.crt";
    std::string server_key  = fs::path(PROJECT_ROOT_DIR) / "mtls/server/server.key";
} // namespace


Server::Server()
: serv_(server_cert.c_str(), server_key.c_str(), ca_cert.c_str())
{
}

void Server::setupRoutes()
{
    // serv_.set_exception_handler(
    // [](const httplib::Request &req, httplib::Response &res, std::exception_ptr ep) {
    //     try { std::rethrow_exception(ep); }
    //     catch (const std::exception &e) {
    //         std::cerr << "Handler exception: " << e.what() << std::endl;
    //     }
    //     res.status = 500;
    //     res.set_content("Internal error", "text/plain");
    // });

    for (const auto& cont : controllers_)
    {
        cont->registerRoute(serv_);
    }
}
