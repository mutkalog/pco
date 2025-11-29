#include "dowloadstateexecutor.h"
#include "../statemachine.h"
#include "../archive.h"
#include "idlestateexecutor.h"
#include "verifyingstateexecutor.h"

#include <filesystem>

namespace fs = std::filesystem;

void DowloadStateExecutor::execute(StateMachine &sm)
{
    auto& ctx = sm.context;

    auto res = ctx.client->Get("/download?type=rpi4&place=machine&id=72");
    std::vector<uint8_t> data(res->body.size());
    std::memcpy(data.data(), res->body.data(), res->body.size());

    if (extract(data.data(), data.size(),
            ctx.testingDir.c_str()) != 0)
    {
        sm.transitTo(&IdleStateExecutor::instance());
        throw std::runtime_error("Cannot extract files");
    }

    for (const auto& file : ctx.manifest.files)
    {
        if (file.installPath != "/")
        {
            fs::path sandboxFilePath = sm.context.sb->getPath() / fs::path(file.installPath).relative_path();

            try
            {
                fs::create_directories(sandboxFilePath.parent_path());
                fs::path filename = sandboxFilePath.filename();
                if (filename.empty()) throw std::runtime_error("Wrong program name");
                fs::copy(ctx.testingDir / filename, sandboxFilePath);
            }
            catch (const std::exception& ex)
            {
                std::cout << ex.what() << std::endl;
                sm.transitTo(&IdleStateExecutor::instance());
                exit(1);
            }
        }
    }

    sm.transitTo(&VerifyingStateExecutor::instance());
}
