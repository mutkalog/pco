#include "downloadcontroller.h"

#include <pqxx/pqxx>


void DownloadController::registerRoute(httplib::Server &serv)
{
    serv.Get("/download", [&](const httplib::Request &req, httplib::Response& res) {

        uint32_t    devId    = std::stoul(req.get_param_value("id"), nullptr, 10);
        std::string type     = req.get_param_value("type");
        std::string arch     = req.get_param_value("arch");
        std::string platform = req.get_param_value("platform");

        std::cout << req.method << " on " << req.path << " from " << arch << " "
                  << type << " on " << platform << " with id " << devId << std::endl;

        try
        {
            std::vector<uint8_t> output = service_->getArchive(sc_, devId, type, platform, arch);
            res.status = httplib::OK_200;
            res.set_content(reinterpret_cast<const char*>(output.data()), output.size(), "application/gzip");
        }
        catch (const pqxx::sql_error& ex)
        {
            res.status = httplib::BadRequest_400;
            res.set_content("Release not found", "text/plain");
        }
        catch (const std::system_error &ex)
        {
            res.status = httplib::InternalServerError_500;
            res.set_content("System error", "text/plain");
        }
        catch (const std::runtime_error &ex)
        {
            res.status = httplib::InternalServerError_500;
            res.set_content("Archive creation failed", "text/plain");
        }
    });
}
