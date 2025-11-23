#include "linuxsandboxinspecor.h"

#include <sys/mount.h>
#include <unistd.h>
#include <fstream>
#include <pwd.h>

#include "../data.h"

void LinuxSandboxInspector::createCgroup(UpdateContext &context)
{
    fs::path basepath = createCgroupsHierarchy();
    fs::path filename = "cont";

    const auto& files = context.manifest.files;

    fs::create_directories(basepath);
    writeToFile(basepath / "cgroup.subtree_control", "+memory +cpu +io");
    fs::create_directories(basepath / filename);

    for (size_t i = 0; i != files.size(); ++i)
    {
        if (files[i].isExecutable == true)
        {
            writeToFile(basepath / filename / "cgroup.procs",
                        std::to_string(context.containeredProcesees[i]));
        }
    }

    std::cout << "PCO cgroup has been created" << std::endl;
}

void LinuxSandboxInspector::inspect(UpdateContext &context)
{
    createCgroup(context);

    /// @todo добавлять процессы в разные cgroup и мониторить их ресурсы в разных потоках
    auto& ctx = context;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::minutes(1);

    uint64_t cpuUsageUsecsPrev = 0;
    uint64_t throttleUsecsPrev = 0;

    const long CORES_COUNT = sysconf(_SC_NPROCESSORS_ONLN);
    const long PAGES_COUNT = sysconf(_SC_PHYS_PAGES);
    const long PAGE_SIZE   = sysconf(_SC_PAGE_SIZE);

    const uint32_t SAMPLE_INTERVAL_USEC = 100e3;
    const uint64_t TOTAL_MEM_BYTES = PAGES_COUNT * PAGE_SIZE;

    double cpuUsageLimitPercentange = 0.82;
    double memUsageLimitPercentange = 0.384;
    double throttleLimitPercentange = 0.01;

    uint64_t memUsageLimitBytes = TOTAL_MEM_BYTES * memUsageLimitPercentange;

    const size_t CPU_SUSTAINED_SAMPLES      = 10;
    const size_t MEM_SUSTAINED_SAMPLES      = 10;
    const size_t THROTTLE_SUSTAINED_SAMPLES = 5;

    size_t cpuExceededPeriods        = 0;
    size_t memExceededPeriods        = 0;
    size_t throttlingExceededPeriods = 0;

    bool decision = true;

    while (std::chrono::steady_clock::now() < deadline &&
           hasFinished() == false)
    {
        auto smpl = sample();

        auto cpuUsageUsecsDelta = smpl.cpuUsage - cpuUsageUsecsPrev;
        double cpuUsageRatio    = cpuUsageUsecsDelta /
                                  (static_cast<double>(SAMPLE_INTERVAL_USEC) * CORES_COUNT);
        cpuUsageUsecsPrev       = smpl.cpuUsage;

        std::cout << "CPU: " <<  cpuUsageRatio << " " << cpuUsageLimitPercentange << std::endl;
        if (cpuUsageRatio > cpuUsageLimitPercentange)
        {
            ++cpuExceededPeriods;
            if (cpuExceededPeriods > CPU_SUSTAINED_SAMPLES)
            {
                decision = false;
                std::cout << "CPU usage_time test failed" << std::endl;
                break;
            }
        }
        else
        {
            cpuExceededPeriods = 0;
        }

        std::cout << "MEM: " <<  smpl.memoryCurrent << " " << memUsageLimitBytes << std::endl;
        if (smpl.memoryCurrent > memUsageLimitBytes)
        {
            ++memExceededPeriods;
            if (memExceededPeriods > MEM_SUSTAINED_SAMPLES)
            {
                decision = false;
                std::cout << "MEM current test failed" << std::endl;
                break;
            }
        }
        else
        {
            memExceededPeriods = 0;
        }

        auto throttleUsecsDelta = smpl.throttledUsec - throttleUsecsPrev;
        std::cout << "THROTTLE: " <<  throttleUsecsDelta << " " << throttleLimitPercentange << std::endl;
        if (throttleUsecsDelta > 0)
        {
            double throttleRatio    = throttleUsecsDelta /
                                      (static_cast<double>(SAMPLE_INTERVAL_USEC) * CORES_COUNT);
            throttleUsecsPrev       = smpl.throttledUsec;

            if (throttleRatio > throttleLimitPercentange)
            {
                ++throttlingExceededPeriods;
                if (throttlingExceededPeriods > THROTTLE_SUSTAINED_SAMPLES)
                {
                    decision = false;
                    std::cout << "CPU throttled_usec test failed" << std::endl;
                    break;
                }
            }
            else
            {
                throttlingExceededPeriods = 0;
            }
        }

        if (smpl.oom > 0)
        {
            decision = false;
            std::cout << "MEM OOM test failed" << std::endl;
            break;
        }

        std::this_thread::sleep_for(std::chrono::microseconds(SAMPLE_INTERVAL_USEC));
    }

    context.finalDecision = decision;
}

void LinuxSandboxInspector::cleanup(UpdateContext &context)
{
    writeToFile(groupdir() / "cgroup.kill", "1");

    while (hasFinished() == false)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    int rc = rmdir(groupdir().c_str());
    if (rc != 0)
    {
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                "Cannot remove cgroup dir " + groupdir().string());
    }

    rc = umount2(groupPath_.c_str(), MNT_DETACH);
    if (rc != 0)
    {
        throw std::runtime_error("Cannot umount cgroup2");
    }

    rc = rmdir(groupPath_.c_str());
    if (rc != 0)
    {
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                "Cannot remove base cgroup dir " + groupdir().string());
    }

    std::cout << groupPath_ << " was successully cleaned" << std::endl;
}

/// перегрузка для случая, когда исполняемые артефакты находятся в разных cgroup
bool LinuxSandboxInspector::hasFinished(pid_t pid, std::string filename)
{
    auto p = Sample::readValue<uint64_t>(groupPath_ / (filename + "/cgroup.procs"));
    return p != pid;
}

/// перегрузка для случая, когда в одной группе находятся все исполняемые артефакты
bool LinuxSandboxInspector::hasFinished()
{
    return Sample::readValue<uint64_t>(groupdir() / "cgroup.procs") == 0;
}

LinuxSandboxInspector::Sample LinuxSandboxInspector::sample()
{
    Sample sample{};
    sample.memoryCurrent  = Sample::readValue<uint64_t>(groupdir() / "memory.current");
    sample.cpuUsage       = Sample::readKeyValue<uint64_t>(groupdir() / "cpu.stat", "usage_usec");
    sample.throttledUsec  = Sample::readKeyValue<uint64_t>(groupdir() / "cpu.stat", "throttled_usec");
    sample.oom            = Sample::readKeyValue<uint64_t>(groupdir() / "memory.events", "oom");

    return sample;
}

fs::path LinuxSandboxInspector::createCgroupsHierarchy()
{
    fs::create_directories(groupPath_);

    int rc = mount(nullptr, groupPath_.c_str(), "cgroup2", 0, nullptr);
    if (rc != 0)
    {
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                "Cannot mount cgroup2");
    }

    return groupPath_;
}

void LinuxSandboxInspector::writeToFile(const fs::path &path, const std::string &value)
{
    std::ofstream file(path);
    if (!file)
        throw std::runtime_error(std::string("Cannot open file") + path.string());

    file.write(value.data(), value.size());
    if (!file)
        throw std::runtime_error(std::string("Cannot write to file") + path.string());
}
