#include "environmentbuildingstateexecutor.h"
#include "../stateexecutors/testingstateexecutor.h"


void EnvironmentBuildingStateExecutor::execute(StateMachine &sm)
{
    auto& sb = sm.context.sb;

    sb->prepare(sm.context);
    sb->launch(sm.context);

    sm.instance().transitTo(&TestingStateExecutor::instance());
}
