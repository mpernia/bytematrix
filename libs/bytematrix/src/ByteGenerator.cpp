#include "ByteGenerator.hpp"

#include <chrono>
#include <random>
#include <thread>
#include <utility>

namespace bytematrix {
    ByteGenerator::~ByteGenerator()
    {
        ByteGenerator::stop();
    }

    void ByteGenerator::setNext(IModule* next)
    {
        next_.store(next, std::memory_order_release);
    }

    bool ByteGenerator::submit([[maybe_unused]] ByteArray data)
    {
        return false;
    }

    void ByteGenerator::start()
    {
        if (running_.exchange(true))
        {
            return;
        }

        if (worker_.joinable())
        {
            worker_.join();
        }

        try
        {
            worker_ = std::thread(&ByteGenerator::run, this);
        }
        catch (...)
        {
            running_ = false;
            throw;
        }
    }

    void ByteGenerator::stop()
    {
        running_ = false;

        if (worker_.joinable())
        {
            worker_.join();
        }
    }

    void ByteGenerator::run()
    {
        std::mt19937 engine(std::random_device{}());
        std::uniform_int_distribution<int> lengthDistribution(1, 100);
        std::uniform_int_distribution<int> byteDistribution(0, 255);

        while (running_)
        {
            ByteArray data(static_cast<std::size_t>(lengthDistribution(engine)));
            for (auto& byte : data)
            {
                byte = static_cast<std::uint8_t>(byteDistribution(engine));
            }

            IModule* next = next_.load(std::memory_order_acquire);
            if (next == nullptr || !next->submit(std::move(data)))
            {
                break;
            }
        }

        running_ = false;
    }
}
