#ifndef BYTEMATRIX_TESTS_SUPPORT_BLOCKING_MODULE_HPP
#define BYTEMATRIX_TESTS_SUPPORT_BLOCKING_MODULE_HPP

#include <bytematrix.h>

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <utility>

namespace bytematrix::test {
    class BlockingModule final : public bytematrix::IModule
    {
    public:
        ~BlockingModule() override
        {
            release();
        }

        void setNext([[maybe_unused]] IModule* next) override {}

        void start() override {}

        void stop() override
        {
            release();
        }

        bool submit([[maybe_unused]] ByteArray data) override
        {
            std::unique_lock<std::mutex> lock(mutex_);
            ++waiting_;
            arrived_.notify_all();
            released_.wait(lock, [this] {
                return open_;
            });
            --waiting_;
            ++delivered_;
            return true;
        }

        [[nodiscard]] bool waitUntilSubmitting(std::chrono::milliseconds timeout)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            return arrived_.wait_for(lock, timeout, [this] {
                return waiting_ > 0;
            });
        }

        void release()
        {
            {
                const std::lock_guard<std::mutex> lock(mutex_);
                open_ = true;
            }

            released_.notify_all();
        }

        [[nodiscard]] std::size_t delivered() const
        {
            const std::lock_guard<std::mutex> lock(mutex_);
            return delivered_;
        }

    private:
        void run() override {}

        mutable std::mutex mutex_;
        std::condition_variable arrived_;
        std::condition_variable released_;
        std::size_t waiting_{0};
        std::size_t delivered_{0};
        bool open_{false};
    };
}

#endif // BYTEMATRIX_TESTS_SUPPORT_BLOCKING_MODULE_HPP
