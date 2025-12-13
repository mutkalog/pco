#ifndef TASK_H
#define TASK_H

#include <atomic>
#include <iostream>
#include <thread>


class Task
{
public:
    Task(const std::string name, int periodSeconds);
    virtual ~Task();

    void start();

protected:
    void virtual process() = 0;

    std::thread          thread_;
    std::atomic<bool>    stopFlag_;
    std::string          name_;
    std::chrono::seconds periodSeconds_;

private:
    void loop();
};

inline Task::Task(const std::string name, int periodSeconds)
    : stopFlag_(false)
    , name_(name)
    , periodSeconds_(std::chrono::seconds(periodSeconds))
{
    std::cout << "Task " << name_ << " has been created" << std::endl;
}

inline Task::~Task()
{
    std::cout << "Task " << name_ << " exited" << std::endl;
    stopFlag_ = true;
    thread_.join();
}

inline void Task::start()
{
    thread_ = std::thread(&Task::loop, this);
}

inline void Task::loop()
{
    while (stopFlag_ == false)
    {
        process();
        std::this_thread::sleep_for(periodSeconds_);
    }
}


#endif // TASK_H
