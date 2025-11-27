#include "environmentbuildingstateexecutor.h"
#include "../stateexecutors/testingstateexecutor.h"
#include "../updatecontext.h"

void EnvironmentBuildingStateExecutor::execute(StateMachine &sm)
{
    auto& sb = sm.context.sb;

    sb->prepare(sm.context);
    // std::cout << "after prepare" << std::endl;
    sb->launch(sm.context);
    // std::cout << "after launch" << std::endl;

    sm.instance().transitTo(&TestingStateExecutor::instance());
}
