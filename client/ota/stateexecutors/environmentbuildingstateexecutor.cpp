#include "environmentbuildingstateexecutor.h"
#include "finalizingstateexecutor.h"
#include "../stateexecutors/testingstateexecutor.h"
#include "../updatecontext.h"
#include <spawn.h>
#include <sys/wait.h>

void PreInstallScriptStateExecutor::execute(StateMachine &sm)
{
    auto& ctx = sm.context;

    try
    {
        fs::path script = ctx.stagingDir / "prepare.sh";
        if (chmod(script.c_str(), 755) != 0)
            throw std::runtime_error("Cannot chmod prepare.sh");

        pid_t childPid = 0;
        char* argv[2] = {const_cast<char*>(script.c_str()), nullptr};

        if (posix_spawn(&childPid, script.c_str(), nullptr, nullptr, argv, environ) != 0)
        {
            throw std::runtime_error("Cannot launch prepare.sh");
        }

        int status = 0;
        if (waitpid(childPid, &status, 0) == -1)
        {
            throw std::runtime_error("Waitpid failed");
        }
        if (WIFEXITED(status) == 0)
        {
            throw std::runtime_error("Wrong status");
        }

        int rc = WEXITSTATUS(status);
        if (rc != 0)
        {
            throw std::runtime_error("prepare.sh returned " + std::to_string(rc));
        }

        sm.instance().transitTo(&TestingStateExecutor::instance());
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
