#include <bytematrix.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <future>
#include <thread>

#include <gtest/gtest.h>

#include "support/blocking_module.hpp"
#include "support/collecting_module.hpp"

namespace {
    using namespace std::chrono_literals;

    using bytematrix::ByteArray;
    using bytematrix::test::BlockingModule;
    using bytematrix::test::CollectingModule;

    constexpr std::size_t kMinimumLength = 1;
    constexpr std::size_t kMaximumLength = 100;

    TEST(ByteGeneratorTest, ProducesPayloadsWithinTheRequiredBounds)
    {
        auto generator = bytematrix::createGenerator();
        CollectingModule sink;
        generator->setNext(&sink);

        generator->start();
        ASSERT_TRUE(sink.waitForAtLeast(300));
        generator->stop();

        for (const ByteArray& payload : sink.payloads())
        {
            EXPECT_GE(payload.size(), kMinimumLength);
            EXPECT_LE(payload.size(), kMaximumLength);
        }
    }

    TEST(ByteGeneratorTest, UsesTheWholeLengthRange)
    {
        auto generator = bytematrix::createGenerator();
        CollectingModule sink;
        generator->setNext(&sink);

        generator->start();
        ASSERT_TRUE(sink.waitForAtLeast(300));
        generator->stop();

        std::size_t shortest = kMaximumLength;
        std::size_t longest = kMinimumLength;
        for (const ByteArray& payload : sink.payloads())
        {
            shortest = std::min(shortest, payload.size());
            longest = std::max(longest, payload.size());
        }

        EXPECT_LT(shortest, 20U) << "the low end of [1..100] must be reachable";
        EXPECT_GT(longest, 80U) << "the high end of [1..100] must be reachable";
    }

    TEST(ByteGeneratorTest, ProducesByteValuesAcrossTheWholeRange)
    {
        auto generator = bytematrix::createGenerator();
        CollectingModule sink;
        generator->setNext(&sink);

        generator->start();
        ASSERT_TRUE(sink.waitForAtLeast(300));
        generator->stop();

        bool seenLow = false;
        bool seenHigh = false;
        for (const ByteArray& payload : sink.payloads())
        {
            for (const std::uint8_t value : payload)
            {
                seenLow = seenLow || value < 16;
                seenHigh = seenHigh || value > 239;
            }
        }

        EXPECT_TRUE(seenLow);
        EXPECT_TRUE(seenHigh);
    }

    TEST(ByteGeneratorTest, StopsDeliveringAfterStopReturns)
    {
        auto generator = bytematrix::createGenerator();
        CollectingModule sink;
        generator->setNext(&sink);

        generator->start();
        ASSERT_TRUE(sink.waitForAtLeast(10));
        generator->stop();

        const std::size_t produced = sink.size();
        std::this_thread::sleep_for(100ms);

        EXPECT_EQ(sink.size(), produced);
    }

    TEST(ByteGeneratorTest, StopIsIdempotent)
    {
        auto generator = bytematrix::createGenerator();
        CollectingModule sink;
        generator->setNext(&sink);

        generator->start();
        ASSERT_TRUE(sink.waitForAtLeast(10));

        generator->stop();
        generator->stop();

        const std::size_t produced = sink.size();
        std::this_thread::sleep_for(50ms);

        EXPECT_EQ(sink.size(), produced);
    }

    TEST(ByteGeneratorTest, StopBeforeStartThenStartStillProduces)
    {
        auto generator = bytematrix::createGenerator();
        CollectingModule sink;
        generator->setNext(&sink);

        generator->stop();
        generator->start();

        EXPECT_TRUE(sink.waitForAtLeast(10));
        generator->stop();
    }

    TEST(ByteGeneratorTest, RestartsWithANewWorkerAfterStop)
    {
        auto generator = bytematrix::createGenerator();
        CollectingModule sink;
        generator->setNext(&sink);

        generator->start();
        ASSERT_TRUE(sink.waitForAtLeast(10));
        generator->stop();

        const std::size_t afterFirstRun = sink.size();

        generator->start();
        EXPECT_TRUE(sink.waitForAtLeast(afterFirstRun + 10))
            << "a stopped generator must be restartable";
        generator->stop();
    }

    TEST(ByteGeneratorTest, StartRepeatedlyIsHarmless)
    {
        auto generator = bytematrix::createGenerator();
        CollectingModule sink;
        generator->setNext(&sink);

        generator->start();
        generator->start();
        generator->start();

        EXPECT_TRUE(sink.waitForAtLeast(10));
        generator->stop();
    }

    TEST(ByteGeneratorTest, DoesNotProduceWithoutADownstream)
    {
        auto generator = bytematrix::createGenerator();

        generator->start();
        generator->stop();

        EXPECT_FALSE(generator->submit(ByteArray{0x00}));
    }

    TEST(ByteGeneratorTest, StopWaitsForAStuckDownstream)
    {
        auto generator = bytematrix::createGenerator();
        BlockingModule sink;
        generator->setNext(&sink);

        generator->start();
        ASSERT_TRUE(sink.waitUntilSubmitting(2s));

        std::promise<void> stopped;
        std::jthread stopper{[&generator, &stopped] {
            generator->stop();
            stopped.set_value();
        }};
        auto done = stopped.get_future();

        const auto outcome = done.wait_for(200ms);
        sink.release();

        EXPECT_EQ(outcome, std::future_status::timeout)
            << "stop() must not return while a downstream is still holding it";
        ASSERT_EQ(done.wait_for(2s), std::future_status::ready)
            << "releasing the downstream must let stop() finish";
    }

    TEST(ByteGeneratorTest, StopReturnsOnceTheDownstreamIsStoppedFirst)
    {
        auto generator = bytematrix::createGenerator();
        BlockingModule sink;
        generator->setNext(&sink);

        generator->start();
        ASSERT_TRUE(sink.waitUntilSubmitting(2s));

        sink.stop();

        std::promise<void> stopped;
        std::jthread stopper{[&generator, &stopped] {
            generator->stop();
            stopped.set_value();
        }};

        EXPECT_EQ(stopped.get_future().wait_for(2s), std::future_status::ready)
            << "stopping the sink first must let the generator stop cleanly";
    }

    TEST(ByteGeneratorTest, StopReturnsPromptlyWhileTheDownstreamAccepts)
    {
        auto generator = bytematrix::createGenerator();
        CollectingModule sink;
        generator->setNext(&sink);
        generator->start();
        ASSERT_TRUE(sink.waitForAtLeast(10));

        std::promise<void> stopped;
        std::jthread stopper{[&generator, &stopped] {
            generator->stop();
            stopped.set_value();
        }};

        EXPECT_EQ(stopped.get_future().wait_for(2s), std::future_status::ready);
    }

    TEST(ByteGeneratorTest, SetNextWhileRunningRedirectsDelivery)
    {
        auto generator = bytematrix::createGenerator();
        CollectingModule first;
        CollectingModule second;
        generator->setNext(&first);

        generator->start();
        ASSERT_TRUE(first.waitForAtLeast(10));

        generator->setNext(&second);

        EXPECT_TRUE(second.waitForAtLeast(10));
        generator->stop();
    }

    TEST(ByteGeneratorTest, RestartsAfterTheWorkerExitedOnItsOwn)
    {
        auto generator = bytematrix::createGenerator();

        generator->start();
        std::this_thread::sleep_for(50ms);

        CollectingModule sink;
        generator->setNext(&sink);
        generator->start();

        EXPECT_TRUE(sink.waitForAtLeast(10));
        generator->stop();
    }
}
