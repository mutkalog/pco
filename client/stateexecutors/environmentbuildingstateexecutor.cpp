#include "environmentbuildingstateexecutor.h"
#include "../sandboxes/linuxsandbox.h"

void EnvironmentBuildingStateExecutor::execute(StateMachine &sm)
{
    LinuxSandbox sb;
    sb.run(sm.context);
    // sb.createRootfs(sm.context);
    // sb.copyDependencies(sm.context);

    exit(0);
}
