#include "idlestateexecutor.h"
#include "checkingstateexecutor.h"

#include <chrono>
#include <thread>


void IdleStateExecutor::execute(StateMachine &sm)
{
    static bool firstTime = true;

    if (firstTime == false)
    {
        std::this_thread::sleep_for(std::chrono::minutes(sm.context.devinfo->pollingIntervalMinutes()));
    }

    firstTime = false;
    sm.transitTo(&CheckingStateExecutor::instance());
}
