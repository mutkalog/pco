#include "finalizingstateexecutor.h"
#include "idlestateexecutor.h"
#include "../deviceinfo.h"
#include <spawn.h>
#include <sys/wait.h>

void FinalizingStateExecutor::execute(StateMachine &sm)
{
    auto& ctx = sm.context;
    bool rebootRequired = false;

    auto safeExec = [&](auto &&fn) {
        try {
            fn();
            return true;
        } catch (const std::exception &ex) {
            std::cout << ex.what() << std::endl;
            ctx.reportMessage.first = INTERNAL_UPDATE_ERROR;
            ctx.reportMessage.second += std::string("\n Error: ") + ex.what();
            return false;
        }
    };

    std::cout << "Final decision is "
              << (ctx.rollback == false ? "UPDATE" : "ROLLBACK")
              << std::endl;

    safeExec([&] {
        if (ctx.rollback == true) {
            rollback(ctx);
        } else {
            ctx.devinfo->saveNewUpdateInfo(ctx.manifest);
            ctx.updateEnvironmentVars();
        }
    });

    bool cleanupOk = safeExec([&]{
        totalCleanup(ctx);
        std::cout << "Cleanup was successfully committed" << std::endl;
    });

    rebootRequired = (cleanupOk == false);

    json results;
    auto& devinfo = sm.context.devinfo;

    results["type"]     = devinfo->type();
    results["arch"]     = devinfo->arch();
    results["platform"] = devinfo->platform();
    results["id"]       = devinfo->id();
    results["status"]   = ctx.rollback == false ? "SUCCESS" : "FAULT";

    results["error"] = {
        { "code",    ctx.reportMessage.first  },
        { "message", ctx.reportMessage.second },
    };

    results["current_version"] = ctx.manifest.release.version;

    ctx.manifest.clear();

    httplib::Result res;
    std::string     body    = results.dump();
    size_t          counter = 0;

    while (res == nullptr && ++counter < 5)
    {
        res = ctx.client->Post("/report", body, "application/json");
        if (counter > 1)
        {
            std::cout << "Server unavailable. "
                         "Trying send result again..." << std::endl;
            std::this_thread::sleep_for(std::chrono::minutes(1));
        }
    }

    if (counter == 5)
    {
        std::cout << "Failed to send report. " << std::endl;
    }

    sm.instance().transitTo(&IdleStateExecutor::instance());

    if (rebootRequired == true)
    {
        std::cout << "REBOOT REBOOT REBOOT" << std::endl;
        exit(EXIT_FAILURE);
        ///@todo вернуть
        // std::system("reboot -r now");
    }
}

void FinalizingStateExecutor::rollback(UpdateContext &ctx)
{
    for (const auto& [path, rollbackPath] : ctx.pathToRollbackPathMap)
    {
        bool havePath     = fs::exists(path);
        bool haveRollback = fs::exists(rollbackPath);

        if (havePath && !haveRollback && ctx.recovering)
        {
            std::cout << "File " + path.string() + " was already rolled back"
                      << std::endl;
            continue;
        }

        std::error_code ec;
        fs::rename(rollbackPath, path, ec);
        if (ec)
        {
            throw std::system_error(ec, "cannot recover app");
        }
    }

    ctx.pathToRollbackPathMap.clear();
    ctx.busyResources.rollbacks = 0;

    const auto it = std::find_if(
        ctx.manifest.files.begin(), ctx.manifest.files.end(),
        [](const auto &item) { return item.installPath == "rollback.sh"; });

    if (it != ctx.manifest.files.end())
    {
        launchScript(ctx.stagingDir / it->installPath.filename());
    }
}

void FinalizingStateExecutor::launchScript(const fs::path& scriptPath)
{
    if (chmod(scriptPath.c_str(), 0755) != 0)
        throw std::runtime_error("Cannot chmod rollback.sh");

    pid_t childPid = 0;
    char* argv[2] = {const_cast<char*>(scriptPath.c_str()), nullptr};

    if (posix_spawn(&childPid, scriptPath.c_str(), nullptr, nullptr, argv, environ) != 0)
    {
        throw std::runtime_error("Cannot launch rollback.sh");
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
        throw std::runtime_error("rollback.sh returned " + std::to_string(rc));
    }
}

void FinalizingStateExecutor::totalCleanup(UpdateContext &ctx)
{
    auto& br = ctx.busyResources;

    if (1 == br.stagingDirCreated)
    {
        std::error_code ec;
        fs::remove_all(ctx.stagingDir);
        if (ec)
            throw std::system_error(ec, "Cannot remove rollback dir");

        br.stagingDirCreated = 0;
    }

    if (1 == br.rollbacks)
    {
        for (const auto& [path, rollbackPath] : ctx.pathToRollbackPathMap)
        {
            std::error_code ec;
            fs::remove_all(rollbackPath, ec);
            if (ec)
                throw std::system_error(ec, "Cannot remove rollback dir");
        }

        ctx.pathToRollbackPathMap.clear();
        br.rollbacks = 0;
    }

    uint32_t word;
    std::memcpy(&word, &ctx.busyResources, sizeof(BusyResources));

    if (word != 0)
        std::cout << "Something hasn't been cleaned! BusyResources word = " +
                         std::to_string(word);

    if (unsetenv("PCO_NEW_ARTIFACTS_PATHS") != 0)
    {
        throw std::system_error(std::error_code(errno, std::generic_category()),
            "Cannot unsetenv PCO_NEW_ARTIFACTS_PATHS");
    }

    if (unsetenv("PCO_STAGING_ARTIFACTS_PATHS") != 0)
    {
        throw std::system_error(std::error_code(errno, std::generic_category()),
            "Cannot unsetenv PCO_STAGING_ARTIFACTS_PATHS");
    }

    ctx.recovering = false;
    ctx.rollback   = false;
}

