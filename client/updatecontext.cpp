#include "updatecontext.h"

#include "deviceinfo.h"

#if defined(__linux__)
#include "sandboxes/linuxsandbox.h"
#include "sandboxes/linuxsandboxinspecor.h"
#endif


UpdateContext::UpdateContext(std::string httpClientSettings)
    : client{httpClientSettings}
    , manifest{}
    , testingDir{"/tmp/quarantine"}
    , signatureOk{}
    , hashsOk{}
    , finalDecision{true}
{
    try
    {
#if defined(__linux__)
        sb      = std::make_unique<LinuxSandbox>("/tmp/quarantine/container");
        sbi     = std::make_unique<LinuxSandboxInspector>();
#endif
        devinfo = std::make_unique<DeviceInfo>();

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
        std::cout << "UpdateContext: There is no prev manifest file. "
                     "Assumnig device has no SW!" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << "UpdateContext: unknown exception -- "
                  << e.what() << std::endl;
        exit(90);
    }
}
