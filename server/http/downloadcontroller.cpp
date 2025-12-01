#include "downloadcontroller.h"

// #include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include "../../utils/common/archive.h"
// namespace fs = std::filesystem;
using json = nlohmann::ordered_json;

namespace {
    std::string testdir = std::string(PROJECT_ROOT_DIR) + "/app4";
} // namespace



void DownloadController::registerRoute(httplib::Server &serv)
{
    serv.Get("/download", [&](const httplib::Request &req, httplib::Response& res) {
        std::cout << req.method << std::endl;
        std::cout << req.path << std::endl;
        std::cout << req.get_param_value("id") << std::endl;
        std::cout << req.get_param_value("type") << std::endl;

        ///@todo получение вектора path из бд
        std::vector<std::string> paths{
            "/opt/pco/storage/Raspberry Pi 4/x86_64/linux/1.2.0/app2",
            "/opt/pco/storage/Raspberry Pi 4/x86_64/linux/1.2.0/app3",
        };

        std::vector<uint8_t> output;

        std::cout << create_archive_from_paths(paths, output) << std::endl;

        // std::ifstream fArchive(testdir + "/app.tar.gz");
        // if (!fArchive) {
        //     throw std::runtime_error("cannot open manifest.json");
        // }

        // std::vector<uint8_t> raw((std::istreambuf_iterator<char>(fArchive)), std::istreambuf_iterator<char>());

        res.status = 200;
        res.set_content(reinterpret_cast<const char*>(output.data()), output.size(), "application/gzip");
    });
}
