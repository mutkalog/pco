#include "processmanager.h"
#include <algorithm>
#include <iostream>
#include <signal.h>


ProcessManager::ProcessManager(std::shared_ptr<MessageQueue<ChildInfo>> messageQueue)
    : stopSupervisor_(false)
    , messageQ_(messageQueue)
{
    supervisorThread_ = std::thread(&ProcessManager::supervisorLoop, this);
}

ProcessManager::~ProcessManager()
{
    terminateAll(2000);
    cv_.notify_all();
    stopSupervisor_ = true;
    messageQ_->shutdown();
    supervisorThread_.join();
}

pid_t ProcessManager::launchProcess(const ArtifactManifest::File& file)
{
    pid_t child = 0;

    auto args = ArtifactManifest::getFileArgs(file);

    int rc = posix_spawn(&child, file.installPath.c_str(), nullptr, nullptr, args.data(), environ);
    if (rc != 0)
    {
        std::cout << "ProcessManager: posix_spawn failed: " << rc << std::endl;
        return -1;
    }

    {
        std::lock_guard<std::mutex> lg(mtx_);
        children_.push_back(ChildInfo{child, file.installPath, -1});
    }

    cv_.notify_all();

    return child;
}

void ProcessManager::terminateAll(int timeoutMillis)
{
    std::vector<pid_t> pids;
    {
        std::lock_guard<std::mutex> lg(mtx_);
        for (auto &c : children_)
            pids.push_back(c.pid);
    }

    if (pids.empty())
        return;

    for (pid_t pid : pids)
        kill(pid, SIGTERM);

    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(timeoutMillis);

    while (std::chrono::steady_clock::now() < deadline)
    {
        {
            std::lock_guard<std::mutex> lg(mtx_);
            if (children_.empty())
                return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    {
        std::lock_guard<std::mutex> lg(mtx_);
        for (auto &c : children_)
        {
            kill(c.pid, SIGKILL);
        }
    }
}

void ProcessManager::supervisorLoop()
{
    while (!stopSupervisor_)
    {
        int status = 0;
        pid_t wp   = waitpid(-1, &status, 0);
        if (wp < 0)
        {
            if (errno == ECHILD)
            {
                std::cout << "ProcessManager: there is no kids" << std::endl;
                std::unique_lock<std::mutex> lk(mtx_);
                cv_.wait_for(lk, std::chrono::milliseconds(1000));
                continue;
            }

            // continue;
            std::cout << "ProcessManager: unkonw waitpid error= " << errno << std::endl;
            break;
        }

        std::lock_guard<std::mutex> lg(mtx_);
        auto it = std::find_if(children_.begin(), children_.end(),
                               [&](const ChildInfo &c) { return c.pid == wp; });

        if (it != children_.end())
        {
            it->lastStatus = status;
            messageQ_->push(std::move(*it));
            std::cout << "ProcessManager: child "
                      << it->name << " with pid = " << it->pid
                      << " exited with status " << it->lastStatus << std::endl;

            children_.erase(it);
        }
        else
        {
            std::cout << "ProcessManager: Reaped unknown child pid=" << wp << std::endl;
        }
    }
}

std::vector<ChildInfo> ProcessManager::listChildren()
{
    std::lock_guard<std::mutex> lg(mtx_);
    return children_;
}
