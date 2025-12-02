#include "dowloadstateexecutor.h"
#include "../statemachine.h"
#include "finalizingstateexecutor.h"
#include "verifyingstateexecutor.h"
#include "../../../utils/common/archive.h"

#include <filesystem>

namespace fs = std::filesystem;

void DowloadStateExecutor::execute(StateMachine &sm)
{
    auto& ctx = sm.context;
    std::string queryString = httplib::encode_uri(
                                std::string("/download?id=") + std::to_string(ctx.devinfo->id())
                                  + "&type=" + ctx.devinfo->type()
                                  + "&arch=" + ctx.devinfo->arch())
                                  + "&platform=" + ctx.devinfo->platform();

    auto res = ctx.client->Get(queryString);
    std::vector<uint8_t> data(res->body.size());
    std::memcpy(data.data(), res->body.data(), res->body.size());

    if (extract(data.data(), data.size(), ctx.testingDir.c_str()) != 0)
    {
        std::string message = "Cannot extract files from archive";
        std::cout << message << std::endl;
        ctx.reportMessage =
        {
            INTERNAL_UPDATE_ERROR,
            message
        };

        sm.transitTo(&FinalizingStateExecutor::instance());
        return;
    }

    ctx.busyResources.testingDirCreated = 1;

    for (const auto& file : ctx.manifest.files)
    {
        if (file.installPath != "/")
        {
            fs::path sandboxFilePath = sm.context.sb->getPath() / fs::path(file.installPath).relative_path();

            try
            {
                fs::create_directories(sandboxFilePath.parent_path());
                fs::path filename = sandboxFilePath.filename();
                if (filename.empty())
                    throw std::runtime_error("Wrong program name");
                fs::copy(ctx.testingDir / filename, sandboxFilePath);
            }
            catch (const std::exception& ex)
            {
                std::cout << ex.what() << std::endl;

                ctx.reportMessage =
                {
                    INTERNAL_UPDATE_ERROR,
                    std::string("Cannot create testing directories\n") + ex.what()
                };

                sm.transitTo(&FinalizingStateExecutor::instance());
                return;
            }
        }
    }

    sm.transitTo(&VerifyingStateExecutor::instance());
}
