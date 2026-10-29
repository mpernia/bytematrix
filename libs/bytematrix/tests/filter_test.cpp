#include <bytematrix.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <future>
#include <initializer_list>
#include <thread>

#include <gtest/gtest.h>

#include "support/blocking_module.hpp"
#include "support/collecting_module.hpp"

namespace {
    using namespace std::chrono_literals;

    using bytematrix::ByteArray;
    using bytematrix::test::BlockingModule;
    using bytematrix::test::CollectingModule;

    ByteArray bytes(std::initializer_list<std::uint8_t> values)
    {
        return ByteArray{values};
    }

    TEST(SequenceFilterTest, ForwardsPayloadsContainingThePattern)
    {
        auto filter = bytematrix::createFilter();
        CollectingModule sink;
        filter->setNext(&sink);
        filter->start();

        const ByteArray matching = bytes({0xAA, 0x00, 0x01, 0x02, 0xBB});
        filter->submit(matching);
        ASSERT_TRUE(sink.waitForAtLeast(1));
        filter->stop();

        ASSERT_EQ(sink.size(), 1U);
        EXPECT_EQ(sink.payloads().front(), matching) << "forwarded unchanged";
    }

    TEST(SequenceFilterTest, DiscardsPayloadsWithoutThePattern)
    {
        auto filter = bytematrix::createFilter();
        CollectingModule sink;
        filter->setNext(&sink);
        filter->start();

        filter->submit(bytes({0x00, 0x01}));
        filter->submit(bytes({0x01, 0x02, 0x00}));
        filter->submit(bytes({0x00}));
        filter->submit(bytes({0xAA, 0x00, 0x01, 0x02}));
        ASSERT_TRUE(sink.waitForAtLeast(1));
        filter->stop();

        EXPECT_EQ(sink.size(), 1U) << "only the payload containing 00 01 02 may pass";
    }

    TEST(SequenceFilterTest, FindsThePatternAtTheStartTheMiddleAndTheEnd)
    {
        auto filter = bytematrix::createFilter();
        CollectingModule sink;
        filter->setNext(&sink);
        filter->start();

        filter->submit(bytes({0x00, 0x01, 0x02, 0xFF}));
        filter->submit(bytes({0xFF, 0x00, 0x01, 0x02, 0xFF}));
        filter->submit(bytes({0xFF, 0x00, 0x01, 0x02}));
        ASSERT_TRUE(sink.waitForAtLeast(3));
        filter->stop();

        EXPECT_EQ(sink.size(), 3U);
    }

    TEST(SequenceFilterTest, AcceptsExactlyCapacityPayloadsBeforeStart)
    {
        auto filter = bytematrix::createFilter();
        std::atomic<bool> submitted{false};

        std::thread producer{[&filter, &submitted] {
            for (std::size_t index = 0; index < bytematrix::kInputBufferCapacity; ++index)
            {
                filter->submit(bytes({0x00, 0x01, 0x02}));
            }

            submitted = true;
        }};

        for (int tick = 0; tick < 200 && !submitted.load(); ++tick)
        {
            std::this_thread::sleep_for(10ms);
        }

        const bool finishedInTime = submitted.load();
        filter->stop();
        producer.join();

        EXPECT_TRUE(finishedInTime) << "the input buffer must hold up to 100 payloads";
    }

    TEST(SequenceFilterTest, AppliesBackpressureWhenTheInputBufferIsFull)
    {
        CollectingModule sink;
        auto filter = bytematrix::createFilter();
        filter->setNext(&sink);

        for (std::size_t index = 0; index < bytematrix::kInputBufferCapacity; ++index)
        {
            filter->submit(bytes({0x00, 0x01, 0x02}));
        }

        std::promise<void> entered;
        std::promise<bool> accepted;
        std::jthread producer{[&filter, &entered, &accepted] {
            entered.set_value();
            accepted.set_value(filter->submit(bytes({0x00, 0x01, 0x02})));
        }};

        entered.get_future().wait();
        auto result = accepted.get_future();

        EXPECT_EQ(result.wait_for(200ms), std::future_status::timeout)
            << "the 101st payload must wait for room";

        filter->start();

        ASSERT_EQ(result.wait_for(2s), std::future_status::ready);
        EXPECT_TRUE(result.get());
        EXPECT_TRUE(sink.waitForAtLeast(bytematrix::kInputBufferCapacity + 1));
        filter->stop();

        EXPECT_EQ(sink.size(), bytematrix::kInputBufferCapacity + 1);
    }

    TEST(SequenceFilterTest, SetNextWhileRunningRedirectsDelivery)
    {
        CollectingModule first;
        CollectingModule second;
        auto filter = bytematrix::createFilter();
        filter->setNext(&first);
        filter->start();

        filter->submit(bytes({0x00, 0x01, 0x02}));
        ASSERT_TRUE(first.waitForAtLeast(1));

        filter->setNext(&second);
        filter->submit(bytes({0x00, 0x01, 0x02}));

        EXPECT_TRUE(second.waitForAtLeast(1));
        filter->stop();
    }

    TEST(SequenceFilterTest, RedirectingWhileProcessingLosesNoPayload)
    {
        CollectingModule first;
        CollectingModule second;
        auto filter = bytematrix::createFilter();
        filter->setNext(&first);
        filter->start();

        constexpr std::size_t kPayloads = 500;
        std::promise<void> entered;
        std::jthread feeder{[&filter, &entered] {
            entered.set_value();
            for (std::size_t index = 0; index < kPayloads; ++index)
            {
                filter->submit(bytes({0x00, 0x01, 0x02}));
            }
        }};

        entered.get_future().wait();

        for (int flip = 0; flip < 1000; ++flip)
        {
            filter->setNext((flip % 2 == 0) ? &second : &first);
        }

        feeder.join();
        filter->stop();

        EXPECT_EQ(first.size() + second.size(), kPayloads);
    }

    TEST(SequenceFilterTest, StopWaitsForAStuckDownstream)
    {
        BlockingModule sink;
        auto filter = bytematrix::createFilter();
        filter->setNext(&sink);
        filter->start();

        ASSERT_TRUE(filter->submit(bytes({0x00, 0x01, 0x02})));
        ASSERT_TRUE(sink.waitUntilSubmitting(2s));

        std::promise<void> stopped;
        std::jthread stopper{[&filter, &stopped] {
            filter->stop();
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

    TEST(SequenceFilterTest, StopReturnsOnceTheDownstreamIsStoppedFirst)
    {
        BlockingModule sink;
        auto filter = bytematrix::createFilter();
        filter->setNext(&sink);
        filter->start();

        ASSERT_TRUE(filter->submit(bytes({0x00, 0x01, 0x02})));
        ASSERT_TRUE(sink.waitUntilSubmitting(2s));

        sink.stop();

        std::promise<void> stopped;
        std::jthread stopper{[&filter, &stopped] {
            filter->stop();
            stopped.set_value();
        }};

        EXPECT_EQ(stopped.get_future().wait_for(2s), std::future_status::ready)
            << "stopping the sink first must let the filter stop cleanly";
    }
}
