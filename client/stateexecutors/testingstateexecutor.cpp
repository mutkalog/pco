#include "testingstateexecutor.h"
#include "../stateexecutors/commitingstateexecutor.h"
#include "../statemachine.h"
#include "../updatecontext.h"

void TestingStateExecutor::execute(StateMachine &sm)
{
    auto& sb = sm.context.sb;

    sm.context.sbi->inspect(sm.context);
    sm.context.sbi->cleanup(sm.context);

    sb->cleanup(sm.context);

    sm.instance().transitTo(&CommitingStateExecutor::instance());
}
