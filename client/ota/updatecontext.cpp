#include "updatecontext.h"
#include "deviceinfo.h"

#if defined(__linux__)
#include "sandboxes/linuxsandbox.h"
#include "sandboxes/linuxsandboxinspecor.h"
#endif


UpdateContext::UpdateContext()
    : manifest{}
    , testingDir{"/tmp/quarantine"}
    , signatureOk{}
    , hashsOk{}
    , finalDecision{true}
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

        ///@todo поменять на настоящие пути
        std::string clientCert = fs::path(PROJECT_ROOT_DIR) / "mtls/client/client.crt";
        std::string clientKey  = fs::path(PROJECT_ROOT_DIR) / "mtls/client/client.key";
        std::string caCert     = fs::path(PROJECT_ROOT_DIR) / "mtls/ca.pem";

        client = std::make_unique<httplib::SSLClient>("localhost", 39024, clientCert, clientKey);
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
            exit(23);
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
        exit(30);
    }
    catch (const std::exception& e)
    {
        std::cout << "UpdateContext: unknown exception -- "
                  << e.what() << std::endl;
        exit(90);
    }
}

// // 1. CA сервера (кому доверяем)
// cli.set_ca_cert_path("/etc/ota/ca.pem");
// cli.enable_server_certificate_verification(true);

// // 2. Клиентский сертификат + ключ (чтобы сервер доверял нам)
// if (!cli.set_client_cert_key(
//         "/etc/ota/client.crt",
//         "/etc/ota/client.key"
//     )) {
//     throw std::runtime_error("cannot load client cert or key");
// }
