#include "commitingstateexecutor.h"
#include "../statemachine.h"

#include "../stateexecutors/idlestateexecutor.h"
#include <filesystem>


namespace fs = std::filesystem;

void CommitingStateExecutor::execute(StateMachine &sm)
{
    auto& ctx = sm.context;

    if (ctx.finalDecision == true)
    {
        std::cout << "Installing new software..." << std::endl;
        for (const auto& file : ctx.manifest.files)
        {
            const auto parentpath = fs::path(file.installPath).parent_path();
            const auto filename   = fs::path(file.installPath).filename();

            fs::create_directories(parentpath);
            // fs::rename(ctx.testingDir / filename, file.installPath);
            fs::copy(ctx.testingDir / filename, file.installPath);
        }
    }

    fs::remove_all(ctx.testingDir);

    ///@todo сохранять файл, содержащий контекст и запускать программу
    ///      далее на этапе download (или придумать другой этап) сравнивать манифест с пришедшим файлом, выключать программы,
    ///      и в случае успеха, на этом этапе класть эти старые программы в одтельную папку со старым манифестом.
    sm.instance().transitTo(&IdleStateExecutor::instance());

    exit(0);
}
