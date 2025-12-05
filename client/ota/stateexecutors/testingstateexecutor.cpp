#include "finalizingstateexecutor.h"
#include "testingstateexecutor.h"
#include "../stateexecutors/commitingstateexecutor.h"
#include "../statemachine.h"
#include "../updatecontext.h"
#include <spawn.h>
#include <sys/wait.h>

void TestingStateExecutor::execute(StateMachine &sm)
{
    auto& ctx = sm.context;
    try
    {
        fs::path script = ctx.stagingDir / "pre-commit.sh";
        if (chmod(script.c_str(), 755) != 0)
            throw std::runtime_error("Cannot chmod " + script.string());

        pid_t childPid = 0;
        char* argv[2] = {const_cast<char*>(script.c_str()), nullptr};

        if (posix_spawn(&childPid, script.c_str(), nullptr, nullptr, argv, environ) != 0)
        {
            throw std::runtime_error("Cannot launch " + script.string());
        }

        int status = 0;
        if (waitpid(childPid, &status, 0) == -1)
        {
            throw std::runtime_error(script.string() + " waitpid failed");
        }
        if (WIFEXITED(status) == 0)
        {
            throw std::runtime_error(script.string() + " wrong status");
        }

        int rc = WEXITSTATUS(status);
        if (rc != 0)
        {
            throw std::runtime_error(script.string() + " returned " + std::to_string(rc));
        }

        sm.context.finalDecision = true;

        sm.instance().transitTo(&CommitingStateExecutor::instance());
    }
    catch (const std::exception& ex)
    {
        ctx.reportMessage.first  = INTERNAL_UPDATE_ERROR;
        std::string message      = ex.what();
        ctx.reportMessage.second += message;

        std::cout << message << std::endl;
        sm.instance().transitTo(&FinalizingStateExecutor::instance());
    }
}
