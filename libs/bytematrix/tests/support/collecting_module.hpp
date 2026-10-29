#ifndef BYTEMATRIX_TESTS_SUPPORT_COLLECTING_MODULE_HPP
#define BYTEMATRIX_TESTS_SUPPORT_COLLECTING_MODULE_HPP

#include "bytematrix.h"

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <utility>
#include <vector>

namespace bytematrix::test {

    inline constexpr auto kDefaultWait = std::chrono::seconds{2};

    class CollectingModule final : public bytematrix::IModule
    {
    public:
        void setNext([[maybe_unused]] IModule* next) override {}

        void start() override {}

        void stop() override
        {
            {
                const std::lock_guard<std::mutex> lock(mutex_);
                stopped_ = true;
            }

            arrived_.notify_all();
        }

        bool submit(ByteArray data) override
        {
            {
                const std::lock_guard<std::mutex> lock(mutex_);
                if (stopped_)
                {
                    return false;
                }

                payloads_.push_back(std::move(data));
            }

            arrived_.notify_all();
            return true;
        }

        [[nodiscard]] bool waitForAtLeast(std::size_t count,
                                          std::chrono::milliseconds timeout = kDefaultWait)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            arrived_.wait_for(lock, timeout, [this, count] {
                return stopped_ || payloads_.size() >= count;
            });

            return payloads_.size() >= count;
        }

        [[nodiscard]] std::size_t size() const
        {
            const std::lock_guard<std::mutex> lock(mutex_);
            return payloads_.size();
        }

        [[nodiscard]] std::vector<ByteArray> payloads() const
        {
            const std::lock_guard<std::mutex> lock(mutex_);
            return payloads_;
        }

    private:
        void run() override {}

        mutable std::mutex mutex_;
        std::condition_variable arrived_;
        std::vector<ByteArray> payloads_;
        bool stopped_{false};
    };
}

#endif // BYTEMATRIX_TESTS_SUPPORT_COLLECTING_MODULE_HPP
