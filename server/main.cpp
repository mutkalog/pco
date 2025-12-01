#include "http/server.h"

#include "http/uploadcontroller.h"
#include "http/registrationcontroller.h"
#include "http/manifestcontrller.h"
#include "http/downloadcontroller.h"
#include "http/reportcontroller.h"

#include "database.h"

#include <vector>


int main()
{
    // Database::instance().connection();

    // std::vector<std::unique_ptr<Controller>> ctrls;
    // ctrls.push_back(std::make_unique<UploadController>());
    // ctrls.push_back(std::make_unique<ManifestContrller>());
    // ctrls.push_back(std::make_unique<DownloadController>());
    // ctrls.push_back(std::make_unique<ReportController>());
    // ctrls.push_back(std::make_unique<RegistrationController>());

    // Server::instance().setControllers(std::move(ctrls));
    // Server::instance().setupRoutes();
    // Server::instance().listen("0.0.0.0", 39024);
        try
    {
        // Database::instance().connection();

        std::vector<std::unique_ptr<Controller>> ctrls;
        ctrls.push_back(std::make_unique<UploadController>());
        ctrls.push_back(std::make_unique<ManifestController>());
        ctrls.push_back(std::make_unique<DownloadController>());
        ctrls.push_back(std::make_unique<ReportController>());
        ctrls.push_back(std::make_unique<RegistrationController>());

        Server::instance().setControllers(std::move(ctrls));
        Server::instance().setupRoutes();
        if (Server::instance().listen("0.0.0.0", 39024) == false)
            throw std::runtime_error("");

        std::cout << "Server started successfully\n";
    }
    catch(const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
}
