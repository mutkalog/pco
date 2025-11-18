#include "linuxsandbox.h"
#include <unistd.h>
#include <filesystem>
#include <sys/mount.h>
#include <sched.h>
#include <unistd.h>
#include <errno.h>
#include <sys/syscall.h>

#include <sched.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>


namespace fs = std::filesystem;

void LinuxSandbox::prepare(const UpdateContext &context)
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

        socketReport(CHILD, OK, "Environment has been built");

        int cmd = socketRead(socketsFds_[CHILD]);

        switch (cmd)
        {
        case RUN:
            for (const auto& app : context.manifest.files)
            {
                if (app.isExecutable == false)
                    continue;

                auto i = app.installPath.rfind('/');
                if (i == std::string::npos)
                    socketReport(CHILD, FAIL, "wrong filename failed");

                std::string programName = app.installPath.substr(i + 1);

                execl(app.installPath.c_str(), programName.c_str(), nullptr);
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
    }
}

void LinuxSandbox::launch(const UpdateContext &context)
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
    std::vector<std::string> rootfs{"bin", "lib", "lib64", "dev", "proc", "tmp", "oldroot"};

    for (const auto& dir : rootfs)
    {
        std::string directory = context.testingDir + "/" + dir;

        if (fs::create_directory(directory) == false)
            throw std::runtime_error("Cannot create rootfs directory \"" + directory + "\"");
    }
}
