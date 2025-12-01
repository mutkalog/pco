#include "uploadcontroller.h"


void UploadController::registerRoute(httplib::Server &serv)
{
    serv.Post("/upload", [&](const httplib::Request& req, httplib::Response& res) {

        if (req.is_multipart_form_data() == false)
        {
            res.status = 400;
            res.set_content("Expected multipart/form-data", "text/plain");
            return;
        }
        std::string rawManifest;
        std::string archive;

        bool gotManifest = false;
        bool gotArchive = false;

        for (const auto& item : req.form.files)
        {
            const auto& file = item.second;

            if (file.content_type == "application/json")
            {
                rawManifest = file.content;
                std::cout << "RAW MANIFEST" << std::endl;
            }

            if (file.content_type == "application/gzip")
            {
                archive = file.content;
                std::cout << "GOT ARCHIVE" << std::endl;
            }
        }

        service_.parseManifest(rawManifest);
        service_.parseFiles(archive);

        try
        {
            service_.commit();
        }
        catch (...)
        {
            res.status = 400;
            res.set_content("Expected manifest and archive", "text/plain");
            return;
        }

        res.status = 200;
        res.set_content("Success", "text/plain");

    });
}
