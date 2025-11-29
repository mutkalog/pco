#include "commitingstateexecutor.h"
#include "../statemachine.h"

#include "../stateexecutors/idlestateexecutor.h"
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

        fs::remove_all(ctx.testingDir);
        sm.instance().transitTo(&IdleStateExecutor::instance());
    };

    if (ctx.finalDecision == true)
    {
        std::cout << "Installing new software..." << std::endl;
        for (const auto &file : ctx.manifest.files)
        {
            const auto parentpath = fs::path(file.installPath).parent_path();
            const auto filename = fs::path(file.installPath).filename();

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
                    onError(std::string("cannot copy ") + file.installPath);
                    return;
                }

                if (fs::remove(file.installPath) == false)
                {
                    onError(std::string("cannot remove ") + file.installPath);
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
                onError(std::string("cannot copy ") +
                        (ctx.testingDir / filename).string());
                return;
            }
        }
        try
        {
            ctx.devinfo->saveNewUpdateInfo(ctx.manifest);
        }
        catch (const std::exception& ex)
        {
            std::cout << ex.what() << std::endl;
            onError("cannot save new manifest file ");
            return;
        }

        std::cout << "Update was successfully commited!" << std::endl;
    }
    else
    {
        std::cout << "Update was canceled!" << std::endl;
    }

    fs::remove_all(ctx.testingDir);

    ///@todo сохранять файл, содержащий контекст и запускать программу
    ///      далее на этапе download (или придумать другой этап) сравнивать манифест с пришедшим файлом, выключать программы,
    ///      и в случае успеха, на этом этапе класть эти старые программы в одтельную папку со старым манифестом.
    sm.instance().transitTo(&IdleStateExecutor::instance());
}
