#ifndef BYTEMATRIX_BYTEGENERATOR_H
#define BYTEMATRIX_BYTEGENERATOR_H

#include "../include/bytematrix.h"

#include <atomic>
#include <thread>

namespace bytematrix {
    class ByteGenerator final : public IModule
    {
    public:
        ByteGenerator() = default;
        ~ByteGenerator() override;

        void setNext(IModule* next) override;
        void start() override;
        void stop() override;
        bool submit(ByteArray data) override;

    private:
        void run() override;

        std::atomic<IModule*> next_{nullptr};
        std::atomic<bool> running_{false};
        std::thread worker_;
    };
};

#endif // BYTEMATRIX_BYTEGENERATOR_H
