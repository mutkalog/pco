#include "updatecontext.h"
#include "deviceinfo.h"

#if defined(__linux__)
#include "sandboxes/linuxsandbox.h"
#include "sandboxes/linuxsandboxinspecor.h"
#endif


UpdateContext::UpdateContext()
    : manifest{}
    , testingDir{"/tmp/quarantine"}
    , finalDecision{false}
    , reportMessage{}
    , busyResources{}
{
    try
    {
#if defined(__linux__)
        sb           = std::make_unique<LinuxSandbox>("/tmp/quarantine/container");
        sbi          = std::make_unique<LinuxSandboxInspector>();
#endif
        devinfo      = std::make_unique<DeviceInfo>();
        supervisorMq = std::make_shared<MessageQueue<ChildInfo>>();
        pm           = std::make_unique<ProcessManager>(supervisorMq);

        std::string clientCert = devinfo->certPath().string();
        std::string clientKey  = devinfo->keyPath().string();
        std::string caCert     = devinfo->caCertPath().string();
        std::string host       = devinfo->serverUrl();
        int         port       = devinfo->serverPort();

        client = std::make_unique<httplib::SSLClient>(host, port, clientCert, clientKey);
        client->set_ca_cert_path(caCert);
        client->enable_server_certificate_verification(true);
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
