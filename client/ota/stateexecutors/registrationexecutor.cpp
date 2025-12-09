#include "registrationexecutor.h"

#include "idlestateexecutor.h"


void RegistrationExecutor::execute(StateMachine &sm)
{
    if (sm.context.devinfo->id() == 0)
    {
        std::cout << "Unregistred device" << std::endl;

        size_t failedAttempts = 0;
        bool   success        = false;
        while (failedAttempts < 5 && success == false)
        {
            if (failedAttempts > 0)
            {
                std::cout << "Another one attempt to register device"
                          << std::endl;
            }

            try
            {
                registerDevice(sm.context);
                success = true;
            }
            catch (const std::exception& ex)
            {
                ++failedAttempts;

                std::cout << ex.what() << std::endl;
                std::cout << "Going to sleep for 1 minute" << std::endl;
                std::this_thread::sleep_for(std::chrono::minutes(1));
            }
        }

        if (failedAttempts == 5)
        {
            std::cout << "Cannot register device. Aborting" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    sm.instance().transitTo(&IdleStateExecutor::instance());
}

void RegistrationExecutor::registerDevice(UpdateContext &ctx)
{
    auto& devinfo = ctx.devinfo;

    json info {
        { "type"          , devinfo->type()                  },
        { "platform"      , devinfo->platform()              },
        { "arch"          , devinfo->arch()                  },
        { "pollingInterval", devinfo->pollingIntervalMinutes() }
    };

    std::string body = info.dump();

    auto res = ctx.client->Post("/register", body, "application/json");

    if (res == nullptr)
    {
        throw std::runtime_error("Server unavailable");
    }

    if (res->status == httplib::OK_200)
    {
        json status = json::parse(res->body);

        if (status["status"] == "registered")
        {
            devinfo->saveId(status["id"]);
            std::cout << "Device registered with id " << devinfo->id() << std::endl;
        }
        else if (status["status"] == "denied")
        {
            std::cout << "Registration denied. Aborting." << std::endl;
            std::cout << res->body << std::endl;
            exit(EXIT_FAILURE);
        }
        else
        {
            throw std::runtime_error("Unknown response");
        }
    }
    else if (res->status == httplib::BadRequest_400)
    {
        std::cout << res->body << std::endl;
        std::cout << "Ill-formed devconfig.json file. Aborting." << std::endl;
        exit(EXIT_FAILURE);
    }
    else
    {
        throw std::runtime_error("Server returned code " + std::to_string(res->status));
    }
}
