#include "finalizingstateexecutor.h"
#include "testingstateexecutor.h"
#include "../stateexecutors/commitingstateexecutor.h"
#include "../statemachine.h"
#include "../updatecontext.h"

void TestingStateExecutor::execute(StateMachine &sm)
{
    auto& ctx = sm.context;
    auto& sbi = ctx.sbi;

    try
    {
        ctx.busyResources.sandboxInspector = 1;
        sbi->inspect(sm.context);
    }
    catch (const std::exception& ex)
    {
        ctx.reportMessage.first  = TEST_FAILED;
        std::string message      = std::string("SandboxInspector: ") + ex.what();
        ctx.reportMessage.second += message;

        std::cout << message << std::endl;
        sm.instance().transitTo(&FinalizingStateExecutor::instance());
        return;
    }

    sm.instance().transitTo(&CommitingStateExecutor::instance());
}
