#include "linuxsandbox.h"
#include <sys/socket.h>
#include <unistd.h>
#include <filesystem>
#include <sys/mount.h>
#include <sched.h>
#include <unistd.h>
#include <errno.h>
#include <sys/syscall.h>

#include <iostream>
#include <sched.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <spawn.h>

#include "../updatecontext.h"


namespace fs = std::filesystem;

void LinuxSandbox::prepare(UpdateContext &context)
{
    createRootfs(context);
    copyDependencies(context);

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, socketsFds_) != 0)
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                "soketpair() failed");

    pid_t pid = fork();
    if (pid == -1)
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                "fork() failed");

    if (pid == 0)
    {

        int rc = unshare(CLONE_NEWNS | CLONE_NEWPID);
        if (rc != 0)
            socketReport(CHILD, FAIL, "unshare() failed");

        rc = mount(nullptr, "/", nullptr, MS_REC | MS_PRIVATE, nullptr);
        if (rc != 0)
            socketReport(CHILD, FAIL, "mount() failed");

        rc = mount(path_.c_str(), path_.c_str(),
                   nullptr, MS_BIND | MS_REC, nullptr);
        if (rc != 0)
            socketReport(CHILD, FAIL, "mount() failed");

        rc = syscall(SYS_pivot_root, path_.c_str(),
                     fs::path(path_ / "oldroot").c_str());
        if (rc != 0)
            socketReport(CHILD, FAIL, "pivot_root() failed");

        rc = mount("proc", "/proc", "proc", 0, nullptr);
        if (rc != 0)
            socketReport(CHILD, FAIL, "/proc mount() failed");

        rc = mount("/dev", "/dev", nullptr, MS_BIND | MS_REC, nullptr);
        if (rc != 0)
            socketReport(CHILD, FAIL, "/dev mount() failed");

        rc = mount("sysfs", "/sys", "sysfs", 0, nullptr);
        if (rc != 0)
            socketReport(CHILD, FAIL, "/sys mount() failed");

        rc = umount2("/oldroot", MNT_DETACH);
        if (rc != 0)
            socketReport(CHILD, FAIL, "umount() failed");

        try {
            fs::remove_all("/oldroot");
        } catch (const fs::filesystem_error &ex) {
            std::cout << ex.what() << std::endl;
        }

        socketReport(CHILD, OK, "Environment has been built");

        int cmd = socketRead(CHILD);

        switch (cmd)
        {
        case RUN:
            for (size_t i = 0; i != context.manifest.files.size(); ++i)
            {
                auto& files = context.manifest.files;

                if (files[i].isExecutable == false)
                    continue;

                fs::path    path        = files[i].installPath;
                std::string programName = path.filename();

                if (programName.empty())
                    socketReport(CHILD, FAIL, "wrong filename failed");

                char* argv[2] = {const_cast<char*>(programName.c_str()), nullptr};

                ////@todo параметры, передаваемые программе
                pid_t childPid = 0;
                if (posix_spawn(&childPid, path.c_str(),
                            nullptr, nullptr, argv, nullptr) != 0)
                {
                    socketReport(CHILD, FAIL,
                                 std::string("Cannot launch ") + path.string());
                }

                socketReport(CHILD, childPid,
                             std::string("Passed child pid ") +
                                 std::to_string(childPid) + " from container");

                int cmd = socketRead(CHILD);
                if (cmd != RUN_NEXT)
                {
                    std::cout << "Received not RUN_NEXT command, aborting launching..." << std::endl;
                    break;
                }
            }
            break;
        case ABORT:
            std::cout << "Abort command received. Aborting" << std::endl;
            exit(0);
            break;
        }
    }
    else
    {
        pid_ = pid;
    }
}

void LinuxSandbox::launch(UpdateContext &context)
{
    int status = socketRead(PARENT);
    switch (status)
    {
    case OK:
        socketReport(PARENT, RUN, "Launch command sent");
        break;
    default:
        break;
    }

    const auto& files = context.manifest.files;
    context.containeredProcesees.resize(files.size());

    for (size_t i = 0; i != files.size(); ++i)
    {
        if (files[i].isExecutable == true)
        {
            context.containeredProcesees[i] = socketRead(PARENT);
            socketReport(PARENT, RUN_NEXT, "Launch next process command sent");
        }
    }
}

void LinuxSandbox::cleanup(UpdateContext &context)
{
    try
    {
        fs::remove_all(path_);
    }
    catch (const fs::filesystem_error &ex)
    {
        std::cout << ex.what() << std::endl;
        // debug-mode
        throw;
    }
}

void LinuxSandbox::copyDependencies(const UpdateContext &context)
{
    ArtifactManifest manifest = context.manifest;

    for (const auto& lib : manifest.requiredSharedLibraries)
    {
        fs::path targetFile = path_ / fs::path(lib).parent_path().relative_path();
        fs::create_directories(targetFile);
        fs::copy(lib, targetFile);
    }

    try
    {
        fs::copy("/bin/busybox", path_ / "bin");
    }
    catch (const std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
    }
}

void LinuxSandbox::createRootfs(const UpdateContext &context)
{
    std::vector<fs::path> rootfs{"bin", "sys", "lib", "lib64", "dev", "proc", "tmp", "oldroot"};

    for (const auto& dir : rootfs)
    {
        fs::path directory = path_ / dir;

        if (fs::create_directory(directory) == false)
            throw std::runtime_error("Cannot create rootfs directory \"" + directory.string() + "\"");
    }
}

void LinuxSandbox::socketReport(int sockFd, int cmd, const std::string &logMessage)
{
    std::cout << logMessage << std::endl;
    std::string msg = std::to_string(cmd);
    if (write(socketsFds_[sockFd], msg.data(), msg.size()) == -1)
    {
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                "write() failed");
    }
    if (cmd == FAIL)
        exit(errno);
}

int LinuxSandbox::socketRead(int sockFd)
{
    std::array<char, sizeof(pid_t) * 2> buf{};
    std::string p = std::to_string(getpid());
    if (read(socketsFds_[sockFd], buf.data(), buf.size()) <= 0)
        throw std::system_error(std::error_code(errno, std::generic_category()),
                                p + " read() failed");

    try
    {
        int cmd = std::stoi(std::string(buf.data()), nullptr, 10);
        return cmd;
    }
    catch (const std::exception &ex)
    {
        std::cerr << ex.what() << std::endl;
        throw;
    }
}
