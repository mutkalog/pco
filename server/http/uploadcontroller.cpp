#include "uploadcontroller.h"

#include "database.h"


void UploadController::registerRoute(httplib::Server &serv)
{
    serv.Post("/upload", [&](const httplib::Request& req, httplib::Response& res) {

        if (req.is_multipart_form_data() == false)
        {
            res.status = httplib::BadRequest_400;
            res.set_content("Expected multipart/form-data", "text/plain");
            return;
        }
        std::string rawManifest;
        std::string archive;

        for (const auto& item : req.form.files)
        {
            const auto& file = item.second;

            if (file.content_type == "application/json")
            {
                rawManifest = file.content;
            }
            else if (file.content_type == "application/gzip")
            {
                archive = file.content;
            }
        }

        try
        {
            service_.upload(rawManifest, archive);
            res.status = httplib::OK_200;
            res.set_content("Success", "text/plain");
        }
        catch (const pqxx::unique_violation& ex)
        {
            std::cout << ex.what() << std::endl;
            res.status = httplib::Conflict_409;
            res.set_content("Such entity already exist on server", "text/plain");
        }
        catch (const pqxx::sql_error& ex)
        {
            std::cout << ex.what() << std::endl;
            res.status = httplib::InternalServerError_500;
            res.set_content("Expected manifest and archive", "text/plain");
        }
        catch (const std::runtime_error& ex)
        {
            std::cout << ex.what() << std::endl;
            res.status = httplib::BadRequest_400;
            res.set_content("Expected manifest and archive", "text/plain");
        }
    });
}
