#include "core/http/server.h"
#include "core/http/server.h"

#include "core/http/uploadcontroller.h"
#include "core/http/registrationcontroller.h"
#include "core/http/manifestcontrller.h"
#include "core/http/downloadcontroller.h"
#include "core/http/reportcontroller.h"

#include "core/service/uploadservice.h"
#include "core/service/registrationservice.h"
#include "core/service/manifestservice.h"
#include "core/service/downloadservice.h"
#include "core/service/reportservice.h"

#include "core/rolloutsupervisor.h"
#include "core/servercontext.h"
#include "core/updatesupervisor.h"

#include "archivetoolsadapter.h"
#include "sslutilsadapter.h"
#include <getopt.h>

#include <vector>

#include <filesystem>
namespace fs = std::filesystem;

namespace {
const fs::path BUFFER_DIR  = "/tmp/pco-buffer";
const fs::path STORAGE_DIR = "/opt/pco/storage";

void parseArgs(int argc, char* argv[], std::string& ca, std::string& cert, std::string& key)
{
    const option long_opts[] = {
        {"ca",   required_argument, nullptr, 'a'},
        {"cert", required_argument, nullptr, 'c'},
        {"key",  required_argument, nullptr, 'k'},
        {nullptr, 0, nullptr, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "a:c:k:", long_opts, nullptr)) != -1)
    {
        switch (opt)
        {
            case 'a': ca   = optarg; break;
            case 'c': cert = optarg; break;
            case 'k': key  = optarg; break;
            default:
                std::cerr << "Usage: "
                          << argv[0]
                          << " --ca <ca> --cert <cert> --key <key>\n";
                std::exit(EXIT_FAILURE);
        }
    }

    if (ca.empty() || cert.empty() || key.empty())
    {
        std::cerr << "All arguments are required\n";
        std::cerr << "Usage: "
                  << argv[0]
                  << " --ca <ca> --cert <cert> --key <key>\n";
        std::exit(EXIT_FAILURE);
    }
}
}


int main(int argc, char* argv[])
{
    std::string caCert;
    std::string cert;
    std::string key;

    parseArgs(argc, argv, caCert, cert, key);

    assert(std::filesystem::exists(caCert));
    assert(std::filesystem::exists(cert));
    assert(std::filesystem::exists(key));

    auto cryptoUtils  = std::make_unique<SSLUtilsAdapter>();
    auto archiveTools = std::make_unique<ArchiveToolsAdapter>();
    auto context      = std::make_shared<ServerContext>(std::move(cryptoUtils), std::move(archiveTools));

    UpdateSupervisor  us(context); us.start();
    RolloutSupervisor rs(context); rs.start();

    std::vector<std::unique_ptr<Controller>> controllers;
    controllers.push_back(std::make_unique<UploadController>(context, std::make_unique<UploadService>(BUFFER_DIR, STORAGE_DIR)));
    controllers.push_back(std::make_unique<ManifestController>(context, std::make_unique<ManifestService>()));
    controllers.push_back(std::make_unique<DownloadController>(context, std::make_unique<DownloadService>()));
    controllers.push_back(std::make_unique<ReportController>(context, std::make_unique<ReportService>()));
    controllers.push_back(std::make_unique<RegistrationController>(context, std::make_unique<RegistrationService>()));

    Server server(caCert, cert, key);
    server.setControllers(std::move(controllers));
    server.setupRoutes();
    server.listen("0.0.0.0", 39024);
}
