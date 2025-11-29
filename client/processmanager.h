#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include <condition_variable>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <sys/wait.h>
#include <spawn.h>
#include <unistd.h>
#include "../utils/messagequeue.h"

struct ChildInfo {
    pid_t pid;
    std::string name;
    int lastStatus; // last wait status
};


class ProcessManager
{
public:
    ProcessManager(std::shared_ptr<MessageQueue<ChildInfo>> messageQueue);

    ~ProcessManager();

    // запустить процесс, вернуть pid или -1
    pid_t launchProcess(const std::string &path, char *const argv[]);

    // мягко завершить все процессы, ждать timeout_ms, затем добивать
    void terminateAll(int timeout_ms);

    // можно читать статусов/список текущих PID
    std::vector<ChildInfo> listChildren();

private:
    std::vector<ChildInfo> children_;
    std::shared_ptr<MessageQueue<ChildInfo>> messageQ_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::thread supervisorThread_;
    std::atomic<bool> stopSupervisor_;

    void supervisorLoop();
};


#endif // PROCESSMANAGER_H
