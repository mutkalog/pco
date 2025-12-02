#include "commitingstateexecutor.h"
#include "../statemachine.h"

#include "finalizingstateexecutor.h"
#include "../updatecontext.h"
#include <filesystem>


namespace fs = std::filesystem;

void CommitingStateExecutor::execute(StateMachine &sm)
{
    auto& ctx = sm.context;

    static auto onError = [&](const std::string& text) {
        std::cout << "Error on istalling new software: "
                  << text
                  << ". Rollback" << std::endl;

        sm.instance().transitTo(&FinalizingStateExecutor::instance());
    };

    if (ctx.finalDecision == true)
    {
        std::cout << "Installing new software..." << std::endl;
        for (const auto &file : ctx.manifest.files)
        {
            const auto parentpath = fs::path(file.installPath).parent_path();
            const auto filename   = fs::path(file.installPath).filename();

            fs::create_directories(parentpath);
            if (fs::exists(file.installPath) == true)
            {
                if (fs::create_directories(parentpath / "rollback-backup") == false &&
                    fs::exists(parentpath / "rollback-backup") == false)
                {
                    onError(std::string("cannot create ") +
                            (parentpath / "rollback-backup").string());
                    return;
                }

                if (fs::exists(parentpath / "rollback-backup" / filename) == true)
                {
                    if (fs::remove(parentpath / "rollback-backup" / filename) == false)
                    {
                        onError(std::string("cannot remove ") +
                                (parentpath / "rollback-backup" / filename).string());
                        return;
                    }
                }

                if (fs::copy_file(file.installPath,
                                  parentpath / "rollback-backup" / filename) == false)
                {
                    onError(std::string("cannot copy ") + file.installPath.string());
                    return;
                }

                if (fs::remove(file.installPath) == false)
                {
                    onError(std::string("cannot remove ") + file.installPath.string());
                    return;
                }
            }

            if (fs::copy_file(ctx.testingDir / filename, file.installPath) == false)
            {
                onError(std::string("cannot copy ") +
                        (ctx.testingDir / filename).string());
                return;
            }

            if (fs::remove(ctx.testingDir / filename) == false)
            {
                onError(std::string("cannot remove ") +
                        (ctx.testingDir / filename).string());
                return;
            }
        }
        try
        {
            ctx.devinfo->saveNewUpdateInfo(ctx.manifest);
            std::cout << "Update was successfully commited!" << std::endl;
        }
        catch (const std::exception& ex)
        {
            std::cout << ex.what() << std::endl;
            onError("cannot save new manifest file ");
            sm.instance().transitTo(&FinalizingStateExecutor::instance());
            return;
        }
    }
    else
    {
        std::cout << "Update was canceled!" << std::endl;
    }

    sm.instance().transitTo(&FinalizingStateExecutor::instance());
}
