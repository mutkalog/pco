#include "finalizingstateexecutor.h"
#include "idlestateexecutor.h"
#include "../deviceinfo.h"

void FinalizingStateExecutor::execute(StateMachine &sm)
{
    auto& ctx = sm.context;

    bool rebootRequired = false;

    try
    {
        totalCleanup(ctx);

        std::cout << "Cleanup was successfully committed" << std::endl;
    }
    catch (const std::exception& ex)
    {
        std::cout << ex.what() << std::endl;

        ctx.reportMessage.first  = INTERNAL_UPDATE_ERROR;
        std::string message      = std::string("\n Cleanup error ") + ex.what();;
        ctx.reportMessage.second += message;
        rebootRequired           = true;
    }

    json results;
    auto& devinfo = sm.context.devinfo;

    results["device"]  = devinfo->type();
    results["status"]  = ctx.finalDecision == true ? "SUCCESS" : "FAULT";

    results["error"] = {
        { "code",    ctx.reportMessage.first  },
        { "message", ctx.reportMessage.second },
    };

    results["current_version"] = devinfo->prevManifest().release.version;

    std::string body = results.dump();

    auto res = ctx.client->Post("/report", body, "application/json");

    if (res && res->status >= 200 && res->status < 300)
    {
        std::cout << "POST successful, status: " << res->status << "\n";
        std::cout << "Response body: " << res->body << "\n";
    }
    else
    {
        if (!res)
        {
            std::cerr << "POST failed: " << static_cast<int>(res.error()) << "\n";
        }
        else
        {
            std::cerr << "POST returned error status: " << res->status << "\n";
            std::cerr << "Response body: " << res->body << "\n";
        }
    }

    if (rebootRequired == true)
    {
        std::cout << "REBOOT REBOOT REBOOT" << std::endl;
        std::cout << "REBOOT REBOOT REBOOT" << std::endl;
        exit(111);
        // std::system("shutdown -r now");
    }

    sm.instance().transitTo(&IdleStateExecutor::instance());
}

void FinalizingStateExecutor::totalCleanup(UpdateContext &ctx)
{
    if (ctx.busyResources.sandboxInspector == 1)
    {
        ctx.sbi->cleanup(ctx);
        ctx.busyResources.sandboxInspector = 0;
    }

    if (ctx.busyResources.sandbox == 1)
    {
        ctx.sb->cleanup(ctx);
        ctx.busyResources.sandbox = 0;
    }

    if (ctx.busyResources.testingDirCreated == 1)
    {
        fs::remove_all(ctx.testingDir);
        ctx.busyResources.testingDirCreated = 0;
    }

    uint16_t word;
    std::memcpy(&word, &ctx.busyResources, sizeof(BusyResources));

    if (word != 0)
        throw std::runtime_error(
            "Something hasn't been cleaned! BusyResources word = " +
            std::to_string(word));
}
