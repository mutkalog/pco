#include "idlestateexecutor.h"
#include "checkingstateexecutor.h"


void IdleStateExecutor::execute(StateMachine &sm)
{
    auto& ctx = sm.context;
    static bool firstTime = true;

    if (firstTime == false)
    {
        ctx.syscalls->sleep(sm.context.devconf->pollingIntervalMinutes() * 60);
    }

    firstTime = false;
    sm.transitTo(CHECKING);
}
