#include "environmentbuildingstateexecutor.h"
#include "finalizingstateexecutor.h"
#include "../stateexecutors/testingstateexecutor.h"
#include "../updatecontext.h"

void EnvironmentBuildingStateExecutor::execute(StateMachine &sm)
{
    auto& ctx = sm.context;
    auto& sb  = ctx.sb;

    try
    {
        ctx.busyResources.sandbox = 1;
        sb->prepare(sm.context);
        sb->launch(sm.context);
    }
    catch (const std::exception& ex)
    {
        ctx.reportMessage.first  = INTERNAL_UPDATE_ERROR;
        std::string message      = std::string("Sandbox: ") + ex.what();
        ctx.reportMessage.second += message;

        std::cout << message << std::endl;
        sm.instance().transitTo(&FinalizingStateExecutor::instance());
        return;
    }

    sm.instance().transitTo(&TestingStateExecutor::instance());
}
