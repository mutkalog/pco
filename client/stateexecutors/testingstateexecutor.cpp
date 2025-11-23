#include "testingstateexecutor.h"

#include "../sandboxes/linuxsandboxinspecor.h"
#include "../statemachine.h"
#include <sys/wait.h>

void TestingStateExecutor::execute(StateMachine &sm)
{
    auto& sb = sm.context.sb;

    const auto& sbi = dynamic_cast<LinuxSandboxInspector*>(sm.context.sbi.get());
    sm.context.sbi->inspect(sm.context);
    sm.context.sbi->cleanup(sm.context);

    // if (sbi != nullptr)
        // sbi->createCgroup(sm.context);


    // auto& ctx = sm.context;
    // auto deadline = std::chrono::steady_clock::now() + std::chrono::minutes(1);
    // pid_t pid = ctx.containeredProcesees[0];
    // // std::string filename = fs::path(ctx.manifest.files[0].installPath).filename();
    // std::string filename = "cont";

    // uint64_t cpuUsageUsecsPrev = 0;
    // uint64_t throttleUsecsPrev = 0;

    // const long CORES_COUNT = sysconf(_SC_NPROCESSORS_ONLN);
    // const long PAGES_COUNT = sysconf(_SC_PHYS_PAGES);
    // const long PAGE_SIZE   = sysconf(_SC_PAGE_SIZE);

    // const uint32_t SAMPLE_INTERVAL_USEC = 100e3;
    // const uint64_t TOTAL_MEM_BYTES = PAGES_COUNT * PAGE_SIZE;

    // double cpuUsageLimitPercentange = 0.69;
    // double memUsageLimitPercentange = 0.25;
    // double throttleLimitPercentange = 0.01;

    // uint64_t memUsageLimitBytes = TOTAL_MEM_BYTES * memUsageLimitPercentange;

    // const size_t CPU_SUSTAINED_SAMPLES      = 10;
    // const size_t MEM_SUSTAINED_SAMPLES      = 10;
    // const size_t THROTTLE_SUSTAINED_SAMPLES = 5;

    // size_t cpuExceededPeriods        = 0;
    // size_t memExceededPeriods        = 0;
    // size_t throttlingExceededPeriods = 0;

    // bool decision = true;

    // const double EPS = 1e-9;

    // while (std::chrono::steady_clock::now() < deadline &&
    //        sbi->hasFinished(pid, filename) == false)
    // {
    //     auto sample = sbi->sample();
    //     // std::cout << sample.memoryCurrent << " " << sample.cpuUsage << " "
    //     //           << sample.throttledUsec << sample.oom << std::endl;

    //     auto cpuUsageUsecsDelta = sample.cpuUsage - cpuUsageUsecsPrev;
    //     double cpuUsageRatio    = cpuUsageUsecsDelta /
    //                               (static_cast<double>(SAMPLE_INTERVAL_USEC) * CORES_COUNT);
    //     cpuUsageUsecsPrev       = sample.cpuUsage;

    //     std::cout << "CPU: " <<  cpuUsageRatio << " " << cpuUsageLimitPercentange << std::endl;
    //     if (std::fabs(cpuUsageRatio - cpuUsageLimitPercentange) < EPS)
    //     {
    //         ++cpuExceededPeriods;
    //         if (cpuExceededPeriods > CPU_SUSTAINED_SAMPLES)
    //         {
    //             decision = false;
    //             std::cout << "CPU usage_time test failed" << std::endl;
    //             sbi->writeToFile(sbi->basepath() / filename / "cgroup.kill", "1");
    //             break;
    //         }
    //         else
    //         {
    //             cpuExceededPeriods = 0;
    //         }
    //     }

    //     std::cout << "MEM: " <<  sample.memoryCurrent << " " << memUsageLimitBytes << std::endl;
    //     if (sample.memoryCurrent > memUsageLimitBytes)
    //     {
    //         ++memExceededPeriods;
    //         if (memExceededPeriods > MEM_SUSTAINED_SAMPLES)
    //         {
    //             decision = false;
    //             std::cout << "MEM current test failed" << std::endl;
    //             sbi->writeToFile(sbi->basepath() / filename / "cgroup.kill", "1");
    //             break;
    //         }
    //     }
    //     else
    //     {
    //         memExceededPeriods = 0;
    //     }

    //     auto throttleUsecsDelta = sample.throttledUsec - throttleUsecsPrev;
    //     std::cout << "THROTTLE: " <<  throttleUsecsDelta << " " << throttleLimitPercentange << std::endl;
    //     if (throttleUsecsDelta > 0)
    //     {
    //         double throttleRatio    = throttleUsecsDelta /
    //                                   (static_cast<double>(SAMPLE_INTERVAL_USEC) * CORES_COUNT);
    //         throttleUsecsPrev       = sample.throttledUsec;

    //         if (std::fabs(throttleRatio - throttleLimitPercentange) < EPS)
    //         {
    //             ++throttlingExceededPeriods;
    //             if (throttlingExceededPeriods > THROTTLE_SUSTAINED_SAMPLES)
    //             {
    //                 decision = false;
    //                 std::cout << "CPU throttled_usec test failed" << std::endl;
    //                 sbi->writeToFile(sbi->basepath() / filename / "cgroup.kill", "1");
    //                 break;
    //             }
    //         }
    //         else
    //         {
    //             throttlingExceededPeriods = 0;
    //         }
    //     }

    //     if (sample.oom > 0)
    //     {
    //         decision = false;
    //         std::cout << "MEM OOM test failed" << std::endl;
    //         sbi->writeToFile(sbi->basepath() / filename / "cgroup.kill", "1");
    //         break;
    //     }

    //     std::this_thread::sleep_for(std::chrono::microseconds(SAMPLE_INTERVAL_USEC));
    // }

    // std::cout << "Testing done" << std::endl;
    sb->cleanup(sm.context);

    exit(0);
}
