#include "downloadcontroller.h"


void DownloadController::registerRoute(httplib::Server &serv)
{
    serv.Get("/download", [&](const httplib::Request &req, httplib::Response& res) {

        uint32_t    devId    = std::stoul(req.get_param_value("id"), nullptr, 10);
        std::string type     = req.get_param_value("type");
        std::string arch     = req.get_param_value("arch");
        std::string platform = req.get_param_value("platform");

        std::cout << req.method << " on " << req.path << " from " << arch << " "
                  << type << " on " << platform << std::endl;

        try
        {
            std::vector<uint8_t> output = service_.getArchive(devId, type, platform, arch);
            res.status = httplib::BadRequest_400;
            res.set_content(reinterpret_cast<const char*>(output.data()), output.size(), "application/gzip");
        }
        catch (const std::system_error &ex)
        {
            throw;
        }
        catch (const std::runtime_error &ex)
        {
            std::cout << ex.what() << std::endl;
            res.status = httplib::BadRequest_400;
            res.set_content("Release not found", "plain/text");
        }
    });
}
