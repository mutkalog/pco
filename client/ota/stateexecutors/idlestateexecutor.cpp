#include "idlestateexecutor.h"
#include "checkingstateexecutor.h"

#include <chrono>
#include <thread>


void IdleStateExecutor::execute(StateMachine &sm)
{
    static bool firstTime = true;

    if (firstTime == false)
    {
        watchProcesses(sm.context, 2000);
    }

    firstTime = false;

    sm.transitTo(&CheckingStateExecutor::instance());
}

void IdleStateExecutor::watchProcesses(UpdateContext& ctx, int gracefullExitMillis)
{
    // auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    auto deadline = std::chrono::steady_clock::now() + std::chrono::minutes(ctx.devinfo->polingIntervalMinutes());
    int failedStartsCount = 0;
    while (true)
    {
        if (ctx.pm->listChildren().empty() == true && ctx.supervisorMq->empty() == true)
        {
            std::cout << "Launching processes" << std::endl;
            for (const auto &file : ctx.devinfo->prevManifest().files)
            {
                if (ctx.pm->launchProcess(file) == -1)
                    throw std::runtime_error("Cannot launch " + file.installPath.string());
            }
        }

        while (ctx.supervisorMq->empty() == true &&
               std::chrono::steady_clock::now() < deadline)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        while (ctx.supervisorMq->empty() == false)
        {
            std::cout << "Some process suddenly died!" << std::endl;
            auto infoOpt = ctx.supervisorMq->pop();
            if (infoOpt.has_value() == true)
            {
                ctx.pm->terminateAll(gracefullExitMillis);
                ///@todo больше 10 -> метка в manifest/deviceinfo, что не работает версия
                ++failedStartsCount;
                std::cout << "Failed starts count: " << failedStartsCount << std::endl;
            }
        }

        if (std::chrono::steady_clock::now() > deadline)
        {
            std::cout << "Deadline exceeded. exiting" << std::endl;
            break;
        }
    }
}


