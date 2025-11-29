#include "environmentbuildingstateexecutor.h"
#include "../stateexecutors/testingstateexecutor.h"
#include "../updatecontext.h"

void EnvironmentBuildingStateExecutor::execute(StateMachine &sm)
{
    auto& sb = sm.context.sb;

    try
    {
        sb->prepare(sm.context);
        sb->launch(sm.context);
    }
    catch (const std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
        ///@todo transit to cleanup stage
        throw;

    }

    sm.instance().transitTo(&TestingStateExecutor::instance());
}
