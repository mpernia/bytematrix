#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <future>
#include <gtest/gtest.h>
#include <set>
#include <thread>
#include <vector>

#include "support/blocking_module.hpp"
#include "support/collecting_module.hpp"
#include <bytematrix.h>

namespace {
    using namespace std::chrono_literals;

    using bytematrix::ByteArray;
    using bytematrix::test::BlockingModule;
    using bytematrix::test::CollectingModule;

    constexpr std::size_t kMinimumLength = 1;
    constexpr std::size_t kMaximumLength = 100;

    std::vector<ByteArray> sample(std::size_t count)
    {
        auto generator = bytematrix::createGenerator();
        CollectingModule sink;
        generator->setNext(&sink);

        generator->start();
        const bool enough = sink.waitForAtLeast(count, 30s);
        generator->stop();

        EXPECT_TRUE(enough) << "the generator stopped delivering before reaching the sample size";

        std::vector<ByteArray> payloads = sink.payloads();
        if (payloads.size() > count)
        {
            payloads.resize(count);
        }

        return payloads;
    }

    std::size_t total_bytes(const std::vector<ByteArray>& payloads)
    {
        std::size_t bytes = 0;
        for (const ByteArray& payload : payloads)
        {
            bytes += payload.size();
        }

        return bytes;
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

    TEST(GeneratorRandomness, ConsecutivePayloadsAreNeverRepeated)
    {
        const std::vector<ByteArray> payloads = sample(1000);
        ASSERT_GT(payloads.size(), 100U);

        std::size_t repeats = 0;
        for (std::size_t index = 1; index < payloads.size(); ++index)
        {
            if (payloads[index].size() < 2 || payloads[index - 1].size() < 2)
            {
                continue;
            }

            if (payloads[index] == payloads[index - 1])
            {
                ++repeats;
            }
        }

        EXPECT_EQ(repeats, 0U) << "consecutive payloads repeated: the engine is not advancing";
    }

    TEST(GeneratorRandomness, GeneratorsCreatedTogetherProduceDifferentStreams)
    {
        auto first = bytematrix::createGenerator();
        auto second = bytematrix::createGenerator();

        CollectingModule firstSink;
        CollectingModule secondSink;
        first->setNext(&firstSink);
        second->setNext(&secondSink);

        first->start();
        second->start();
        ASSERT_TRUE(firstSink.waitForAtLeast(20, 10s));
        ASSERT_TRUE(secondSink.waitForAtLeast(20, 10s));
        first->stop();
        second->stop();

        std::vector<ByteArray> firstPayloads = firstSink.payloads();
        std::vector<ByteArray> secondPayloads = secondSink.payloads();
        firstPayloads.resize(20);
        secondPayloads.resize(20);

        EXPECT_NE(firstPayloads, secondPayloads) << "two generators shared a seed";
    }

    TEST(GeneratorRandomness, ByteValuesAreUniformlyDistributed)
    {
        const std::vector<ByteArray> payloads = sample(500);
        ASSERT_GT(payloads.size(), 400U);

        std::array<std::size_t, 256> histogram{};
        for (const ByteArray& payload : payloads)
        {
            for (const std::uint8_t value : payload)
            {
                ++histogram[value];
            }
        }

        const std::size_t bytes = total_bytes(payloads);
        ASSERT_GT(bytes, 20000U);

        auto* const lowest = std::ranges::min_element(histogram);
        auto* const highest = std::ranges::max_element(histogram);
        const std::size_t expected = bytes / histogram.size();

        EXPECT_GT(*lowest, expected / 2)
            << "byte value " << (lowest - histogram.begin()) << " is under-represented";
        EXPECT_LT(*highest, expected * 2)
            << "byte value " << (highest - histogram.begin()) << " is over-represented";
    }

    TEST(GeneratorRandomness, BytesVaryWithinASinglePayload)
    {
        const std::vector<ByteArray> payloads = sample(300);
        ASSERT_GT(payloads.size(), 250U);

        std::size_t considered = 0;
        std::size_t uniform = 0;
        for (const ByteArray& payload : payloads)
        {
            if (payload.size() < 8)
            {
                continue;
            }

            ++considered;
            const bool uniformPayload =
                std::ranges::all_of(payload, [first = payload.front()](std::uint8_t value) {
                    return value == first;
                });

            if (uniformPayload)
            {
                ++uniform;
            }
        }

        ASSERT_GT(considered, 100U);
        EXPECT_EQ(uniform, 0U) << "a payload was filled with a single repeated byte value";
    }

    TEST(GeneratorRandomness, LengthsAreUniformlyDistributed)
    {
        const std::vector<ByteArray> payloads = sample(5000);
        ASSERT_GT(payloads.size(), 4900U);

        std::array<std::size_t, kMaximumLength + 1> histogram{};
        std::size_t outside = 0;
        for (const ByteArray& payload : payloads)
        {
            if (payload.size() < kMinimumLength || payload.size() > kMaximumLength)
            {
                ++outside;
                continue;
            }

            ++histogram[payload.size()];
        }

        EXPECT_EQ(outside, 0U) << "a length outside [1..100] was generated";

        std::size_t lowest = payloads.size();
        std::size_t highest = 0;
        for (std::size_t length = kMinimumLength; length <= kMaximumLength; ++length)
        {
            EXPECT_GT(histogram[length], 0U) << "length " << length << " was never generated";
            lowest = std::min(lowest, histogram[length]);
            highest = std::max(highest, histogram[length]);
        }

        const std::size_t expected = payloads.size() / kMaximumLength;
        EXPECT_GT(lowest, expected / 4) << "some lengths are far under-represented";
        EXPECT_LT(highest, expected * 2) << "some lengths are far over-represented";
    }

    TEST(GeneratorRandomness, LongPayloadsDoNotRepeat)
    {
        const std::vector<ByteArray> payloads = sample(1000);
        ASSERT_GT(payloads.size(), 900U);

        std::set<ByteArray> distinct;
        std::size_t longPayloads = 0;
        for (const ByteArray& payload : payloads)
        {
            if (payload.size() < 8)
            {
                continue;
            }

            ++longPayloads;
            distinct.insert(payload);
        }

        EXPECT_GT(longPayloads, 500U);
        EXPECT_EQ(distinct.size(), longPayloads);
    }

    TEST(GeneratorRandomness, TheStatementPatternAlmostNeverAppears)
    {
        constexpr std::size_t kPayloads = 50000;
        const std::array<std::uint8_t, 3> kPattern{0x00, 0x01, 0x02};

        const std::vector<ByteArray> payloads = sample(kPayloads);
        ASSERT_GT(payloads.size(), kPayloads - 100);

        std::size_t hits = 0;
        for (const ByteArray& payload : payloads)
        {
            if (!std::ranges::search(payload, kPattern).empty())
            {
                ++hits;
            }
        }

        const std::size_t bytes = total_bytes(payloads);
        std::printf("[          ] %zu payloads, %zu bytes, %zu occurrences of 00 01 02 "
                    "(expected ~%.2f)\n",
                    payloads.size(), bytes, hits, static_cast<double>(bytes) / 16777216.0);

        EXPECT_LE(hits, 4U) << "the three-byte pattern is supposed to be rare";
    }
}
