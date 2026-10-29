#ifndef BYTEMATRIX_QUEUEDMODULE_HPP
#define BYTEMATRIX_QUEUEDMODULE_HPP

#include "../include/bytematrix.h"
#include "detail/queue.hpp"

#include <atomic>
#include <cstddef>
#include <thread>
#include <utility>

namespace bytematrix {
    class QueuedModule : public IModule
    {
    public:
        explicit QueuedModule(std::size_t capacity) : input_(capacity) {}

        ~QueuedModule() override;

        bool submit(ByteArray data) override
        {
            return input_.push(std::move(data));
        }

        void start() override;
        void stop() override;

    protected:
        virtual void process(ByteArray data) = 0;
        virtual void onStop() {}

        detail::Queue<ByteArray> input_;

    private:
        void run() override;

        std::atomic<bool> started_{false};
        std::thread worker_;
    };
}

#endif // BYTEMATRIX_QUEUEDMODULE_HPP
