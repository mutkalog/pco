// #include "linuxsandboxinspecor.h"

// #include <sys/mount.h>
// #include <unistd.h>
// #include <fstream>
// #include <pwd.h>

// #include "../updatecontext.h"

// void LinuxSandboxInspector::createCgroup(UpdateContext &context)
// {
//     fs::path basepath = createCgroupsHierarchy();
//     fs::path filename = "cont";

//     const auto& files = context.manifest.files;

//     writeToFile(basepath / "cgroup.subtree_control", "+memory +cpu +io");
//     fs::create_directories(basepath / filename);
//     busyResources_.groupDirCreated = 1;

//     for (size_t i = 0; i != files.size(); ++i)
//     {
//         if (files[i].isExecutable == true)
//         {
//             writeToFile(basepath / filename / "cgroup.procs",
//                         std::to_string(context.containeredProcesees[i]), true);

//             busyResources_.processesAdded = 1;
//         }
//     }

//     std::cout << "LinuxSandboxInspector: cgroup has been created" << std::endl;
// }

// void LinuxSandboxInspector::inspect(UpdateContext &context)
// {
//     createCgroup(context);
//     using namespace std::chrono;

//     auto& testReqs = context.manifest.testRequirments;

//     auto     testingTimeSeconds = seconds(testReqs.timeSeconds);
//     auto     testingTimeUsecs   = duration_cast<microseconds>(testingTimeSeconds);
//     uint64_t messagesStep       = testingTimeUsecs.count() / 10;
//     auto     deadline           = steady_clock::now() + testingTimeSeconds;

//     double   cpuAvgRation = 0;
//     double   memAvgUsage = 0;
//     double   throttleAvgRatio = 0;

//     uint64_t cpuUsageUsecsPrev = 0;
//     uint64_t throttleUsecsPrev = 0;

//     const long CORES_COUNT = sysconf(_SC_NPROCESSORS_ONLN);
//     const long PAGES_COUNT = sysconf(_SC_PHYS_PAGES);
//     const long PAGE_SIZE   = sysconf(_SC_PAGE_SIZE);

//     const uint64_t SAMPLE_INTERVAL_USEC = 100e3;
//     const uint64_t TOTAL_MEM_BYTES = PAGES_COUNT * PAGE_SIZE;

//     double cpuUsageLimitPercentange = testReqs.cpuLimitPercentage == 0
//                                           ? 1.0
//                                           : testReqs.cpuLimitPercentage / 100.0;
//     double memUsageLimitPercentange = testReqs.memLimitPercentage == 0
//                                           ? 1.0
//                                           : testReqs.memLimitPercentage / 100.0;
//     double throttleLimitPercentange = testReqs.throttleLimitPercentage == 0
//                                           ? 1.0
//                                           : testReqs.throttleLimitPercentage / 100.0;

//     uint64_t memUsageLimitBytes = TOTAL_MEM_BYTES * memUsageLimitPercentange;

//     const size_t CPU_SUSTAINED_SAMPLES      = 10;
//     const size_t MEM_SUSTAINED_SAMPLES      = 10;
//     const size_t THROTTLE_SUSTAINED_SAMPLES = 5;

//     size_t cpuExceededPeriods        = 0;
//     size_t memExceededPeriods        = 0;
//     size_t throttlingExceededPeriods = 0;

//     bool decision = true;

//     uint64_t elapsedUsecs    = 0;
//     uint64_t lastReportUsecs = 0;
//     uint64_t samplesCount    = 0;

//     std::cout << "LinuxSandboxInspector: Software test has been started" << std::endl;

//     while (std::chrono::steady_clock::now() < deadline &&
//            hasFinished() == false)
//     {
//         auto smpl = sample();

//         auto cpuUsageUsecsDelta = smpl.cpuUsage - cpuUsageUsecsPrev;
//         double cpuUsageRatio    = cpuUsageUsecsDelta /
//                                   (static_cast<double>(SAMPLE_INTERVAL_USEC) * CORES_COUNT);
//         cpuUsageUsecsPrev       = smpl.cpuUsage;
//         cpuAvgRation        += cpuUsageRatio;

//         if (cpuUsageRatio > cpuUsageLimitPercentange)
//         {
//             ++cpuExceededPeriods;
//             if (cpuExceededPeriods > CPU_SUSTAINED_SAMPLES)
//             {
//                 decision              = false;
//                 std::string message   = "CPU usage_time test failed";
//                 context.reportMessage = {TEST_FAILED, message};

//                 std::cout << message << std::endl;
//                 break;
//             }
//         }
//         else
//         {
//             cpuExceededPeriods = 0;
//         }

//         memAvgUsage += smpl.memoryCurrent;
//         if (smpl.memoryCurrent > memUsageLimitBytes)
//         {
//             ++memExceededPeriods;
//             if (memExceededPeriods > MEM_SUSTAINED_SAMPLES)
//             {
//                 decision              = false;
//                 std::string message   = "MEM current test failed";
//                 context.reportMessage = {TEST_FAILED, message};

//                 std::cout << message << std::endl;
//                 break;
//             }
//         }
//         else
//         {
//             memExceededPeriods = 0;
//         }

//         auto throttleUsecsDelta = smpl.throttledUsec - throttleUsecsPrev;
//         if (throttleUsecsDelta > 0)
//         {
//             double throttleRatio = throttleUsecsDelta /
//                                    (static_cast<double>(SAMPLE_INTERVAL_USEC) * CORES_COUNT);
//             throttleUsecsPrev    = smpl.throttledUsec;
//             throttleAvgRatio     += throttleRatio;

//             if (throttleRatio > throttleLimitPercentange)
//             {
//                 ++throttlingExceededPeriods;
//                 if (throttlingExceededPeriods > THROTTLE_SUSTAINED_SAMPLES)
//                 {
//                     decision              = false;
//                     std::string message   = "CPU throttled_usec test failed";
//                     context.reportMessage = {TEST_FAILED, message};

//                     std::cout << message << std::endl;
//                     break;
//                 }
//             }
//             else
//             {
//                 throttlingExceededPeriods = 0;
//             }
//         }

//         if (smpl.oom > 0)
//         {
//             decision              = false;
//             std::string message   = "MEM OOM test failed";
//             context.reportMessage = {TEST_FAILED, message};

//             std::cout << message << std::endl;
//             break;
//         }

//         std::this_thread::sleep_for(std::chrono::microseconds(SAMPLE_INTERVAL_USEC));

//         elapsedUsecs += SAMPLE_INTERVAL_USEC;
//         ++samplesCount;

//         if (elapsedUsecs - lastReportUsecs >= messagesStep)
//         {
//             lastReportUsecs = elapsedUsecs;
//             uint32_t percent = (elapsedUsecs * 100) / testingTimeUsecs.count();

//             std::cout << "LinuxSandboxInspector: progress: " << percent << "%" << std::endl;;
//             std::cout << "\tCPU average usage: " <<  cpuAvgRation / samplesCount << std::endl;
//             std::cout << "\tMEM average usage: " <<  (memAvgUsage / samplesCount) / TOTAL_MEM_BYTES<< std::endl;
//             std::cout << "\tTHROTTLE average usage: " <<  throttleAvgRatio / samplesCount << std::endl;
//         }
//     }

//     if (hasFinished())
//     {
//         // for (const auto& p : context.containeredProcesees)
//         // {
//             std::cout << "LinuxSandboxInspector: process "  << " exited before test end" << std::endl;
//             context.finalDecision = false;

//             // int status = context.sb->getReturnCode().second;
//             // if (WIFEXITED(status) != 0)
//             //     decision &= WEXITSTATUS(status) == 0;
//             // else
//             //     decision = false;
//         // }
//     }
//     else if (steady_clock::now() >= deadline)
//     {
//         context.finalDecision = decision;
//         std::cout << "LinuxSandboxInspector: test passed" << std::endl;
//     }
//     else if (testingTimeSeconds.count() == 0)
//     {
//         context.finalDecision = decision;
//         std::cout << "LinuxSandboxInspector: test skipped due to "
//                      "zero ArtifactManifest.testRequirments.timeSeconds" << std::endl;
//     }
//     else
//     {
//         std::cout << "LinuxSandboxInspector: test failed" << std::endl;
//         context.finalDecision = false;
//     }
// }

// void LinuxSandboxInspector::cleanup(UpdateContext &context)
// {
//     if (busyResources_.processesAdded == 1)
//     {
//         writeToFile(groupdir() / "cgroup.kill", "1");

//         while (hasFinished() == false)
//         {
//             std::this_thread::sleep_for(std::chrono::milliseconds(10));
//         }
//         std::cout << "LinuxSandboxInspector: processes was killed" << std::endl;
//     }

//     busyResources_.processesAdded = 0;

//     if (busyResources_.groupDirCreated == 1)
//     {
//         int rc = rmdir(groupdir().c_str());
//         if (rc != 0)
//         {
//             throw std::system_error(std::error_code(errno, std::generic_category()),
//                                     "Cannot remove cgroup dir " + groupdir().string());
//         }
//         std::cout << "LinuxSandboxInspector: groupDir was removed" << std::endl;
//     }

//     busyResources_.groupDirCreated = 0;

//     if (busyResources_.cgroupMounted == 1)
//     {
//         int rc = umount2(groupPath_.c_str(), MNT_FORCE | MNT_DETACH);
//         if (rc != 0)
//         {
//             throw std::runtime_error("Cannot umount cgroup2");
//         }
//         std::cout << "LinuxSandboxInspector: groupPath was unmounted" << std::endl;
//     }

//     busyResources_.cgroupMounted = 0;

//     if (busyResources_.groupDirCreated == 1)
//     {
//         int rc = rmdir(groupPath_.c_str());
//         if (rc != 0)
//         {
//             throw std::system_error(std::error_code(errno, std::generic_category()),
//                                     "Cannot remove base cgroup dir " + groupdir().string());
//         }
//         std::cout << "LinuxSandboxInspector: group dir was removed" << std::endl;
//     }
//     busyResources_.groupDirCreated = 0;

//     std::cout << "LinuxSandboxInspector: " <<  groupPath_ << " was successully cleaned" << std::endl;
// }

// /// перегрузка для случая, когда исполняемые артефакты находятся в разных cgroup
// bool LinuxSandboxInspector::hasFinished(pid_t pid, std::string filename)
// {
//     auto p = Sample::readValue<uint64_t>(groupPath_ / (filename + "/cgroup.procs"));
//     return p != pid;
// }

// /// перегрузка для случая, когда в одной группе находятся все исполняемые артефакты
// bool LinuxSandboxInspector::hasFinished()
// {
//     return Sample::readValue<uint64_t>(groupdir() / "cgroup.procs") == 0;
// }

// LinuxSandboxInspector::Sample LinuxSandboxInspector::sample()
// {
//     Sample sample{};
//     sample.memoryCurrent  = Sample::readValue<uint64_t>(groupdir() / "memory.current");
//     sample.cpuUsage       = Sample::readKeyValue<uint64_t>(groupdir() / "cpu.stat", "usage_usec");
//     sample.throttledUsec  = Sample::readKeyValue<uint64_t>(groupdir() / "cpu.stat", "throttled_usec");
//     sample.oom            = Sample::readKeyValue<uint64_t>(groupdir() / "memory.events", "oom");

//     return sample;
// }

// fs::path LinuxSandboxInspector::createCgroupsHierarchy()
// {
//     fs::create_directories(groupPath_);
//     busyResources_.groupPathCreated = 1;

//     int rc = mount(nullptr, groupPath_.c_str(), "cgroup2", 0, nullptr);
//     if (rc != 0)
//     {
//         throw std::system_error(std::error_code(errno, std::generic_category()),
//                                 "Cannot mount cgroup2");
//     }

//     busyResources_.cgroupMounted = 1;
//     return groupPath_;
// }

// void LinuxSandboxInspector::writeToFile(const fs::path &path, const std::string &value, bool append)
// {

//     std::ofstream file(path, append == true
//                                  ? (std::ios_base::app | std::ios_base::out)
//                                  : (std::ios_base::out));
//     if (!file)
//         throw std::runtime_error(std::string("Cannot open file") + path.string());

//     file.write(value.data(), value.size());
//     if (!file)
//         throw std::runtime_error(std::string("Cannot write to file") + path.string());
// }
