#ifndef MESSAGEQUEUE_H
#define MESSAGEQUEUE_H

#include <iostream>
#include <optional>
#include <queue>
#include <condition_variable>

template <typename T>
class MessageQueue
{
public:

    template <typename U>
    void push(U&& val)
    {
        {
            std::unique_lock ul(m_);
            q_.emplace(std::forward<U>(val));
        }

        itemPushedCv_.notify_one();
        std::cout << "MessageQueue: pushed message" << std::endl;
    }

    std::optional<T>
    pop() noexcept
    {
        std::unique_lock ul(m_);
        itemPushedCv_.wait(ul, [this]() { return !q_.empty() || stopped_; });

        if (stopped_ && q_.empty())
            return std::nullopt;

        T item = std::move(q_.front());
        q_.pop();

        std::cout << "MessageQueue: poped message" << std::endl;
        return std::make_optional(item);
    }

    bool empty() noexcept
    {
        std::lock_guard lg(m_);
        return q_.empty();
    }

    void shutdown() noexcept
    {
        {
            std::lock_guard lg(m_);
            stopped_ = true;
        }

        itemPushedCv_.notify_all();
    }

private:
    std::queue<T> q_;
    std::mutex m_;
    std::condition_variable itemPushedCv_;
    bool stopped_ = false;
};

#endif
