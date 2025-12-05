#include "updatecontext.h"
#include "deviceinfo.h"

namespace {
    const fs::path PCO_STAGING_DIR = "/tmp/pco/staging";
}


UpdateContext::UpdateContext()
    : manifest{}
    , stagingDir{PCO_STAGING_DIR}
    , finalDecision{false}
    , reportMessage{}
    , busyResources{}
{
    try
    {
        devinfo      = std::make_unique<DeviceInfo>();

        std::string clientCert = devinfo->certPath().string();
        std::string clientKey  = devinfo->keyPath().string();
        std::string caCert     = devinfo->caCertPath().string();
        std::string host       = devinfo->serverUrl();
        int         port       = devinfo->serverPort();

        client = std::make_unique<httplib::SSLClient>(host, port, clientCert, clientKey);
        client->set_ca_cert_path(caCert);
        client->enable_server_certificate_verification(true);

        if (setenv("PCO_STAGING_DIR", PCO_STAGING_DIR.c_str(), 1) != 0)
        {
            throw std::system_error(std::error_code(errno, std::generic_category()),
                                    "Cannot setenv PCO_STAGING_DIR");
        }
        else if (fs::create_directories(PCO_STAGING_DIR) == false && fs::exists(PCO_STAGING_DIR) == false)
        {
            throw std::system_error(std::error_code(errno, std::generic_category()),
                                    "Cannot create PCO_STAGING_DIR");
        }
    }
    catch (const nlohmann::detail::exception& e)
    {
        std::cout << e.what() << std::endl;
        std::cout << "UpdateContext: JSON parse error" << std::endl;
        if (devinfo->type().empty() == true
            || devinfo->serverUrl().empty() == true)
        {
            std::cout << "UpdateContext: Cannot continue without critical device info."
                         " Aborting..." << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    catch (const std::system_error& e)
    {
        std::cout << "UpdateContext: something went wrong."
                     " Aborting" << std::endl;
        std::cout << e.what() << " " << e.code().message() << std::endl;
        exit(e.code().value());
    }
    catch (const std::runtime_error& e)
    {

        std::cout << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
    catch (const std::exception& e)
    {
        std::cout << "UpdateContext: unknown exception -- "
                  << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
}
