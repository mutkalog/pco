#include "testingstateexecutor.h"

#include "../sandboxes/linuxsandboxinspecor.h"
#include "../statemachine.h"
#include <sys/wait.h>

void TestingStateExecutor::execute(StateMachine &sm)
{
    auto& sb = sm.context.sb;

    const auto& sbi = dynamic_cast<LinuxSandboxInspector*>(sm.context.sbi.get());

    if (sbi != nullptr)
        sbi->createCgroup(sm.context);


    auto& ctx = sm.context;
    // auto hasFinished = [&](pid_t pid) -> bool {
    //     int rc, status;
    //     rc = waitpid(pid, &status, WNOHANG);
    //     if (rc == 0)
    //     {
    //         return false;
    //     }
    //     else if (rc == pid)
    //     {
    //         std::cout << "Process "
    //                   << pid << " exited" +
    //                   (WIFEXITED(status) != 0
    //                                ? " normaly with code " + std::to_string(WEXITSTATUS(status))
    //                               : " abnormaly") << std::endl;
    //         return true;
    //     }
    //     else if (rc == -1)
    //     {
    //         std::cout << "Process already dead" << std::endl;
    //         return true;
    //     }
    // };

    auto deadline = std::chrono::steady_clock::now() + std::chrono::minutes(1);
    pid_t pid = ctx.containeredProcesees[0];
    std::string filename = fs::path(ctx.manifest.files[0].installPath).filename();

    uint64_t cpuUsageUsecsPrev = 0;
    uint64_t throttleUsecsPrev = 0;

    const long CORES_COUNT = sysconf(_SC_NPROCESSORS_ONLN);
    const long PAGES_COUNT = sysconf(_SC_PHYS_PAGES);
    const long PAGE_SIZE   = sysconf(_SC_PAGE_SIZE);

    const uint32_t SAMPLE_INTERVAL_USEC = 100e3;
    const uint64_t TOTAL_RAM_BYTES = PAGES_COUNT * PAGE_SIZE;

    // std::cout << coresCount << std::endl;
    // std::cout << totalRam << std::endl;

    double cpuUsageLimitPercentage = 0.8;
    bool descision = true;

    while (std::chrono::steady_clock::now() < deadline &&
           sbi->hasFinished(pid, filename) == false)
    {
        auto sample = sbi->sample();
        // std::cout << sample.memoryCurrent << " " << sample.cpuUsage << " "
        //           << sample.throttledUsec << sample.oom << std::endl;


        auto cpuUsageUsecsDelta = sample.cpuUsage - cpuUsageUsecsPrev;
        double cpuUsageRatio = cpuUsageUsecsDelta /
                             static_cast<double>(SAMPLE_INTERVAL_USEC * 1.0);


        cpuUsageUsecsPrev = sample.cpuUsage;
        std::cout << (cpuUsageRatio / CORES_COUNT) << " " << cpuUsageLimitPercentage << std::endl;
        if ((cpuUsageRatio / CORES_COUNT) > cpuUsageLimitPercentage)
        {
            descision = false;
            std::cout << "CPU usage\_time test failed" << std::endl;
            sbi->writeToFile(sbi->basepath() / filename / "cgroup.kill", "1");
            break;
        }

        std::this_thread::sleep_for(std::chrono::microseconds(SAMPLE_INTERVAL_USEC));
    }

    std::cout << "Testing done" << std::endl;
    sb->cleanup(sm.context);

    exit(0);
}
