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

#include "../data.h"


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

        rc = mount(context.testingDir.c_str(), context.testingDir.c_str(),
                   nullptr, MS_BIND | MS_REC, nullptr);
        if (rc != 0)
            socketReport(CHILD, FAIL, "mount() failed");

        rc = syscall(SYS_pivot_root, context.testingDir.c_str(),
                     std::string(context.testingDir + "/" + "oldroot").c_str());
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

        // while (true) sleep(1);
        switch (cmd)
        {
        case RUN:
            // for (const auto& app : context.manifest.files)
            for (size_t i = 0; i != context.manifest.files.size(); ++i)
            {
                auto& files = context.manifest.files;

                if (files[i].isExecutable == false)
                    continue;

                fs::path    path        = files[i].installPath;
                std::string programName = path.filename();

                if (programName.empty())
                    socketReport(CHILD, FAIL, "wrong filename failed");

                // execl(app.installPath.c_str(), programName.c_str(), nullptr);
                char* argv[2] = {const_cast<char*>(programName.c_str()), nullptr};

                pid_t childPid = 0;
                if (posix_spawn(&childPid, path.c_str(),
                            nullptr, nullptr, argv, nullptr) != 0)
                {
                    std::cout << "FAIL in spawn" << std::endl;
                    socketReport(CHILD, FAIL,
                                 std::string("Cannot launch ") + path.string());
                }

                socketReport(CHILD, childPid,
                             std::string("Passed child pid ") +
                                 std::to_string(childPid) + " from container");

                std::cout << "PID IS " <<  context.containeredProcesees[i] << std::endl;
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
        containerPid_ = pid;
        std::cout << pid << std::endl;
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
        }
    }
}

void LinuxSandbox::cleanup(UpdateContext &context)
{
    sleep(2);
    try {
        fs::remove_all(context.testingDir);
    } catch (const fs::filesystem_error &ex) {
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
        std::string targetFile = context.testingDir + lib.substr(0, lib.rfind("/"));
        fs::create_directories(targetFile);
        fs::copy(lib, targetFile);
    }

    try
    {
        fs::copy("/bin/busybox", context.testingDir + "/" + "bin");
        fs::copy("/lib/x86_64-linux-gnu/libresolv.so.2", context.testingDir + "/lib/x86_64-linux-gnu");

        std::cout << "busybox copied to container" << std::endl;
    }
    catch (const std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
    }
}

void LinuxSandbox::createRootfs(const UpdateContext &context)
{
    std::vector<std::string> rootfs{"bin", "sys", "lib", "lib64", "dev", "proc", "tmp", "oldroot"};

    for (const auto& dir : rootfs)
    {
        std::string directory = context.testingDir + "/" + dir;

        if (fs::create_directory(directory) == false)
            throw std::runtime_error("Cannot create rootfs directory \"" + directory + "\"");
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
