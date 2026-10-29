#ifndef BYTEMATRIX_QUEUE_HPP
#define BYTEMATRIX_QUEUE_HPP

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <utility>

namespace bytematrix::detail {
    template <typename T>
    class Queue
    {
    public:
        explicit Queue(std::size_t capacity) : capacity_(capacity) {}

        Queue(const Queue&) = delete;
        Queue& operator=(const Queue&) = delete;
        Queue(Queue&&) = delete;
        Queue& operator=(Queue&&) = delete;

        bool push(T value)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            notFull_.wait(lock, [this] {
                return closed_ || queue_.size() < capacity_;
            });

            if (closed_)
            {
                return false;
            }

            queue_.push_back(std::move(value));
            notEmpty_.notify_one();
            return true;
        }

        bool pop(T& value)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            notEmpty_.wait(lock, [this] {
                return closed_ || !queue_.empty();
            });

            if (queue_.empty())
            {
                return false;
            }

            value = std::move(queue_.front());
            queue_.pop_front();
            notFull_.notify_one();
            return true;
        }

        void close()
        {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                closed_ = true;
            }

            notEmpty_.notify_all();
            notFull_.notify_all();
        }

        [[nodiscard]] std::size_t capacity() const noexcept
        {
            return capacity_;
        }

    private:
        const std::size_t capacity_;
        std::deque<T> queue_;
        bool closed_{false};
        std::mutex mutex_;
        std::condition_variable notEmpty_;
        std::condition_variable notFull_;
    };
}

#endif // BYTEMATRIX_QUEUE_HPP
