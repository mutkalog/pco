#include "commitingstateexecutor.h"
#include "../statemachine.h"

#include "../stateexecutors/idlestateexecutor.h"
#include "../updatecontext.h"
#include <filesystem>


namespace fs = std::filesystem;

void CommitingStateExecutor::execute(StateMachine &sm)
{
    auto& ctx = sm.context;

    static auto onError = [&]() {
        std::cout << "Error on istalling new software. "
                    "Rollback" << std::endl;

        fs::remove_all(ctx.testingDir);
        sm.instance().transitTo(&IdleStateExecutor::instance());
    };

    if (ctx.finalDecision == true)
    {
        std::cout << "Installing new software..." << std::endl;
        for (const auto& file : ctx.manifest.files)
        {
            const auto parentpath = fs::path(file.installPath).parent_path();
            const auto filename   = fs::path(file.installPath).filename();

            fs::create_directories(parentpath);
            // fs::rename(ctx.testingDir / filename, file.installPath);
            if (fs::exists(ctx.testingDir / filename) == true)
            {
                if (fs::create_directories(parentpath / "rollback-backup") == false
                    && fs::exists(parentpath / "rollback-backup" / filename) == true)
                {
                    if (fs::remove(parentpath / "rollback-backup" / filename) == false)
                        onError();

                    if (fs::copy_file(file.installPath,
                                      parentpath / "rollback-backup" / filename) == false)
                        onError();

                    if (fs::remove(file.installPath) == false)
                        onError();

                }

                if (fs::copy_file(ctx.testingDir / filename, file.installPath) == false)
                    onError();

                if (fs::remove(file.installPath) == false)
                    onError();

            }


            // fs::copy(ctx.testingDir / filename, file.installPath);
        }
    }

    fs::remove_all(ctx.testingDir);

    try
    {
        ctx.devinfo->saveNewUpdateInfo(ctx.manifest);
    }
    catch (const std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
    }

    ///@todo сохранять файл, содержащий контекст и запускать программу
    ///      далее на этапе download (или придумать другой этап) сравнивать манифест с пришедшим файлом, выключать программы,
    ///      и в случае успеха, на этом этапе класть эти старые программы в одтельную папку со старым манифестом.
    sm.instance().transitTo(&IdleStateExecutor::instance());

    exit(0);
}
