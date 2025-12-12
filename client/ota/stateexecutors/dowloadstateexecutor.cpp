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

    try
    {
        std::string     queryString = httplib::encode_uri(
                                       std::string("/download?id=") + std::to_string(ctx.devinfo->id())
                                       + "&type=" + ctx.devinfo->type()
                                       + "&arch=" + ctx.devinfo->arch())
                                       + "&platform=" + ctx.devinfo->platform();
        httplib::Result res;
        size_t          counter     = 0;

        while ((res == nullptr || res->status != httplib::OK_200)
               && ++counter < 5)
        {
            res = ctx.client->Get(queryString);
            if (counter > 1)
            {
                std::cout << "Server unavailable. "
                             "Trying send request again..." << std::endl;
                std::this_thread::sleep_for(std::chrono::minutes(1));
            }
        }

        if (counter == 5)
        {
            throw std::runtime_error("Cannot get artifacts: server is down");
        }

        process(sm, res->body);
    }
    catch (const std::exception& ex)
    {
        ctx.rollback = true;
        std::cout << ex.what() << std::endl;
        sm.transitTo(&FinalizingStateExecutor::instance());
    }
}

void DowloadStateExecutor::process(StateMachine &sm, const std::string &responseBody)
{
    auto& ctx = sm.context;

    std::vector<uint8_t> data(responseBody.size());
    std::memcpy(data.data(), responseBody.data(), responseBody.size());

    if (extract(data.data(), data.size(), ctx.stagingDir.c_str()) != 0)
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

    ctx.busyResources.stagingDirCreated = 1;

    sm.transitTo(&VerifyingStateExecutor::instance());
}
