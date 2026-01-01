#include "core/stateexecutors/preparingstateexecutor.h"
#include "core/updatecontext.h"
#include <spawn.h>
#include <sys/wait.h>
#include <stdio.h>


void PreparingStateExecutor::execute(StateMachine &sm)
{
    auto& ctx = sm.context;

    try
    {
        fs::path script = ctx.stagingDir / "prepare.sh";
        if (ctx.syscalls->chmod(script.c_str(), 0755) != 0)
            throw std::runtime_error("Cannot chmod prepare.sh");

        pid_t childPid = 0;
        char* argv[2] = {const_cast<char*>(script.c_str()), nullptr};

        if (ctx.syscalls->posix_spawn(&childPid, script.c_str(), nullptr, nullptr, argv, environ) != 0)
        {
            perror("cannot cannot");
            throw std::runtime_error("Cannot launch prepare.sh");
        }

        int status = 0;
        if (ctx.syscalls->waitpid(childPid, &status, 0) == -1)
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

        std::cout << "Preparing script exited normaly" << std::endl;
        sm.transitTo(INSTALLING);
    }
    catch (const std::exception& ex)
    {
        ctx.rollback = true;
        ctx.reportMessage.first  = INTERNAL_UPDATE_ERROR;
        std::string message      = ex.what();
        ctx.reportMessage.second += message;

        std::cout << message << std::endl;
        sm.transitTo(FINALIZING);
    }
}
