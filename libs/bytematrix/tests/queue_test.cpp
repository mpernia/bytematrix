#include "../include/bytematrix.h"

#include "../src/detail/queue.hpp"

#include <chrono>
#include <cstdint>
#include <future>
#include <initializer_list>
#include <thread>

#include <gtest/gtest.h>

namespace {
    using namespace std::chrono_literals;

    using bytematrix::ByteArray;
    using bytematrix::detail::Queue;

    ByteArray bytes(std::initializer_list<std::uint8_t> values)
    {
        return ByteArray{values};
    }

    TEST(QueueTest, PopsInFifoOrder)
    {
        Queue<int> queue{4};

        EXPECT_TRUE(queue.push(1));
        EXPECT_TRUE(queue.push(2));
        EXPECT_TRUE(queue.push(3));

        int value = 0;
        ASSERT_TRUE(queue.pop(value));
        EXPECT_EQ(value, 1);
        ASSERT_TRUE(queue.pop(value));
        EXPECT_EQ(value, 2);
        ASSERT_TRUE(queue.pop(value));
        EXPECT_EQ(value, 3);
    }

    TEST(QueueTest, ReportsTheCapacityRequiredByTheStatement)
    {
        EXPECT_EQ(Queue<int>{bytematrix::kInputBufferCapacity}.capacity(), 100U);
    }

    TEST(QueueTest, PopBlocksUntilAnItemArrives)
    {
        Queue<ByteArray> queue{4};
        ByteArray received;
        std::thread consumer{[&queue, &received] {
            queue.pop(received);
        }};

        queue.push(bytes({0x00, 0xFF}));
        consumer.join();

        EXPECT_EQ(received, bytes({0x00, 0xFF}));
    }

    TEST(QueueTest, PushBlocksWhileFullAndResumesWhenDrained)
    {
        Queue<int> queue{2};
        ASSERT_TRUE(queue.push(1));
        ASSERT_TRUE(queue.push(2));

        std::promise<void> entered;
        std::promise<bool> pushed;
        std::jthread producer{[&queue, &entered, &pushed] {
            entered.set_value();
            pushed.set_value(queue.push(3));
        }};

        entered.get_future().wait();
        auto result = pushed.get_future();
        EXPECT_EQ(result.wait_for(200ms), std::future_status::timeout)
            << "the third item must wait for room";

        int value = 0;
        ASSERT_TRUE(queue.pop(value));

        EXPECT_EQ(result.wait_for(1s), std::future_status::ready);
        EXPECT_TRUE(result.get());
        ASSERT_TRUE(queue.pop(value));
        EXPECT_EQ(value, 2);
        ASSERT_TRUE(queue.pop(value));
        EXPECT_EQ(value, 3);
    }

    TEST(QueueTest, CloseWakesBlockedProducerAndRejectsFurtherPushes)
    {
        Queue<int> queue{1};
        ASSERT_TRUE(queue.push(1));

        std::promise<void> entered;
        std::promise<bool> pushed;
        std::jthread producer{[&queue, &entered, &pushed] {
            entered.set_value();
            pushed.set_value(queue.push(2));
        }};

        entered.get_future().wait();
        auto result = pushed.get_future();
        EXPECT_EQ(result.wait_for(200ms), std::future_status::timeout)
            << "push must block while the queue is full";

        queue.close();

        EXPECT_EQ(result.wait_for(1s), std::future_status::ready);
        EXPECT_FALSE(result.get());
        EXPECT_FALSE(queue.push(3));
    }

    TEST(QueueTest, DrainsBufferedItemsBeforeReportingClosure)
    {
        Queue<int> queue{4};
        ASSERT_TRUE(queue.push(10));
        ASSERT_TRUE(queue.push(20));

        queue.close();

        int value = 0;
        ASSERT_TRUE(queue.pop(value));
        EXPECT_EQ(value, 10);
        ASSERT_TRUE(queue.pop(value));
        EXPECT_EQ(value, 20);
        EXPECT_FALSE(queue.pop(value));
        EXPECT_FALSE(queue.pop(value)) << "closure is observable more than once";
    }
}
