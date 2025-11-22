#include "linuxsandboxinspecor.h"

#include <sys/mount.h>
#include <unistd.h>
#include <fstream>
#include <pwd.h>

#include "../data.h"

void LinuxSandboxInspector::createCgroup(UpdateContext &context)
{
    fs::path basepath = createCgroupsHierarchy();

    const auto& files = context.manifest.files;

    for (size_t i = 0; i != files.size(); ++i)
    {
        if (files[i].isExecutable == true)
        {
            fs::path filename = fs::path(files[i].installPath).filename();
            try
            {
                fs::create_directories(basepath);

                // writeToFile(basepath / "cgroup.controllers", "+memory +cpu +io");
                writeToFile(basepath / "cgroup.subtree_control", "+memory +cpu +io");

                fs::create_directories(basepath / filename);
                std::string s = basepath / filename;
                writeToFile(basepath / filename / "cgroup.procs",
                            std::to_string(context.containeredProcesees[i]));
            }
            catch (const std::exception& ex)
            {
                std::cout << ex.what() << std::endl;
                throw;
            }
        }
    }

    std::cout << "PCO cgroup has been created" << std::endl;
}

void LinuxSandboxInspector::inspect(UpdateContext &context)
{

}

bool LinuxSandboxInspector::hasFinished(pid_t pid, std::string filename)
{
    auto p = Sample::readInt(groupPath_ / (filename + "/cgroup.procs"));
    return p != pid;
}

LinuxSandboxInspector::Sample LinuxSandboxInspector::sample()
{
    Sample sample{};
    sample.memoryCurrent  = Sample::readMemory(groupPath_);
    sample.cpuUsage       = Sample::readCpu(groupPath_);
    sample.throttledUsec  = Sample::readThrottle(groupPath_);
    sample.throttledUsec  = Sample::readThrottle(groupPath_);

    return sample;
}

fs::path LinuxSandboxInspector::createCgroupsHierarchy()
{
    fs::path basepath = "/sys/fs/cgroup/pco";
    fs::create_directories(basepath);

    // int rc = mount(nullptr, basepath.c_str(), "cgroup2", 0, nullptr);
    // if (rc != 0)
    // {
    //     throw std::runtime_error("Cannot mount cgroup2");
    // }
    return basepath;

}

void LinuxSandboxInspector::writeToFile(const fs::path &path, const std::string &value)
{
    // std::ofstream file(path);
    // if (!file)
    //     throw std::runtime_error(std::string("Cannot open file") + path.string());

    // file.write(value.data(), value.size());
    // if (!file)
    //     throw std::runtime_error(std::string("Cannot write to file") + path.string());

    int fd = open(path.c_str(), O_WRONLY);
    if (fd == -1) {
        close(fd);
        throw std::runtime_error(std::string("Cannot open file") + path.string());
    }

    if (write(fd, value.c_str(), value.size()) == -1) {
        close(fd);
        perror("cannot write");
        throw std::runtime_error(std::string("Cannot write to file") + path.string());
    }

    close(fd);
}

uint64_t LinuxSandboxInspector::Sample::readMemory(const std::filesystem::__cxx11::path &path)
{
    std::ifstream file(path.string() + "/memory.current");
    if (!file)
        throw std::runtime_error("Cannot open: " + path.string());

    uint64_t value{};
    file >> value;

    return value;
}

uint64_t LinuxSandboxInspector::Sample::readCpu(const std::filesystem::__cxx11::path &path)
{
    std::ifstream file(path.string() + "/cpu.stat");
    if (!file)
        throw std::runtime_error("Cannot open: " + path.string());

    std::string key;
    uint64_t value;

    while (file >> key >> value)
    {
        if (key == "usage_usec")
            return value;
    }

    throw std::runtime_error("Cannot find usage_usec in cpu.stat");
}

uint64_t LinuxSandboxInspector::Sample::readThrottle(const std::filesystem::__cxx11::path &path)
{
    std::ifstream file(path.string() + "/cpu.stat");
    if (!file)
        throw std::runtime_error("Cannot open: " + path.string());

    std::string key;
    uint64_t value;

    while (file >> key >> value)
    {
        if (key == "throttled_usec")
            return value;
    }

    throw std::runtime_error("Cannot find throttled_usec in cpu.stat");
}

bool LinuxSandboxInspector::Sample::readOom(const std::filesystem::__cxx11::path &path)
{
    std::ifstream file(path.string() + "/memory.events");
    if (!file)
        throw std::runtime_error("Cannot open: " + path.string());

    std::string key;
    uint64_t value;

    while (file >> key >> value)
    {
        if (key == "oom" && value > 0)
            return true;
    }

    return false;
}

uint64_t LinuxSandboxInspector::Sample::readInt(const fs::path &path)
{
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("Cannot open: " + path.string());

    uint64_t value;
    file >> value;
    file >> value;

    return value;
}
