#include "dowloadstateexecutor.h"
#include "../statemachine.h"
#include "../archive.h"
#include "idlestateexecutor.h"
#include "verifyingstateexecutor.h"

void DowloadStateExecutor::execute(StateMachine &sm)
{
    auto& ctx = sm.context;

    auto res = ctx.client.Get("/download?type=rpi4&place=machine&id=72");
    std::vector<uint8_t> data(res->body.size());
    std::memcpy(data.data(), res->body.data(), res->body.size());

    if (extract(data.data(), data.size(),
            ctx.testingDir.c_str()) != 0)
    {
        sm.transitTo(&IdleStateExecutor::instance());
        throw std::runtime_error("Cannot extract files");
    }

    // std::cout << "DONE" << std::endl;
    // exit(0);

    sm.transitTo(&VerifyingStateExecutor::instance());
}
