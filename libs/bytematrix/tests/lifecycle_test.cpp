#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <initializer_list>
#include <memory>
#include <thread>
#include <vector>

#include "support/collecting_module.hpp"
#include <bytematrix.h>

namespace {
    using namespace std::chrono_literals;

    using bytematrix::ByteArray;

    ByteArray bytes(std::initializer_list<std::uint8_t> values)
    {
        return ByteArray{values};
    }

    TEST(LifecycleTest, StartFromSeveralThreadsIsSafe)
    {
        auto filter = bytematrix::createFilter();
        bytematrix::test::CollectingModule sink;
        filter->setNext(&sink);

        std::atomic<int> ready{0};
        std::vector<std::thread> starters;
        starters.reserve(8);

        for (int index = 0; index < 8; ++index)
        {
            starters.emplace_back([&filter, &ready] {
                ready.fetch_add(1);
                while (ready.load() < 8)
                {
                    std::this_thread::yield();
                }

                filter->start();
            });
        }

        for (std::thread& starter : starters)
        {
            starter.join();
        }

        filter->submit(bytes({0x00, 0x01, 0x02}));
        ASSERT_TRUE(sink.waitForAtLeast(1));
        filter->stop();

        EXPECT_EQ(sink.size(), 1U) << "a payload must be processed exactly once";
    }

    TEST(LifecycleTest, StartIsIdempotent)
    {
        auto filter = bytematrix::createFilter();
        bytematrix::test::CollectingModule sink;
        filter->setNext(&sink);

        filter->start();
        filter->start();

        filter->submit(bytes({0x00, 0x01, 0x02}));
        ASSERT_TRUE(sink.waitForAtLeast(1));
        filter->stop();

        EXPECT_EQ(sink.size(), 1U);
    }

    TEST(LifecycleTest, StopIsIdempotent)
    {
        auto filter = bytematrix::createFilter();
        filter->start();
        filter->stop();
        filter->stop();

        SUCCEED();
    }

    TEST(LifecycleTest, DestructorStopsTheInternalThread)
    {
        bytematrix::test::CollectingModule sink;

        {
            auto filter = bytematrix::createFilter();
            filter->setNext(&sink);
            filter->start();
            filter->submit(bytes({0x00, 0x01, 0x02}));
        }

        EXPECT_EQ(sink.size(), 1U) << "the destructor drains the buffer before joining";
    }

    TEST(LifecycleTest, StopDrainsAlreadyBufferedInput)
    {
        auto filter = bytematrix::createFilter();
        bytematrix::test::CollectingModule sink;
        filter->setNext(&sink);

        for (std::size_t index = 0; index < 25; ++index)
        {
            filter->submit(bytes({0x00, 0x01, 0x02}));
        }

        filter->start();
        filter->stop();

        EXPECT_EQ(sink.size(), 25U);
    }

    TEST(LifecycleTest, SubmitAfterStopIsDroppedWithoutBlocking)
    {
        auto filter = bytematrix::createFilter();
        bytematrix::test::CollectingModule sink;
        filter->setNext(&sink);
        filter->start();
        filter->stop();

        const auto started = std::chrono::steady_clock::now();
        for (std::size_t index = 0; index < bytematrix::kInputBufferCapacity * 10; ++index)
        {
            filter->submit(bytes({0x00, 0x01, 0x02}));
        }
        const auto elapsed = std::chrono::steady_clock::now() - started;

        EXPECT_EQ(sink.size(), 0U) << "a stopped module must not deliver";
        EXPECT_LT(elapsed, 1s) << "a stopped module must not block the producer";
    }

    TEST(LifecycleTest, WorksWithoutADownstreamModule)
    {
        auto filter = bytematrix::createFilter();
        filter->start();
        filter->submit(bytes({0x00, 0x01, 0x02}));
        filter->stop();

        SUCCEED() << "an unwired output must be a no-op, not a crash";
    }
}
