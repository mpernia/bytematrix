#include <chrono>
#include <cstdio>
#include <future>
#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "support/collecting_module.hpp"
#include "support/cout_capture.hpp"
#include "support/records.hpp"

namespace {
    using namespace std::chrono_literals;

    using bytematrix::ByteArray;
    using bytematrix::test::CollectingModule;
    using bytematrix::test::CoutCapture;
    using bytematrix::test::lines_of;
    using bytematrix::test::payload_of;
    using bytematrix::test::record_line_count;
    using bytematrix::test::timestamp_of;

    const std::string kRecord = "2026-10-29T04:00:00Z 3 bytes=[0x00, 0x01, 0x02]";

    TEST(LinesOfTest, ReturnsNothingForEmptyOutput)
    {
        EXPECT_TRUE(lines_of("").empty());
    }

    TEST(LinesOfTest, DoesNotAddALineForTheTrailingNewline)
    {
        EXPECT_EQ(lines_of("a\nb\n"), (std::vector<std::string>{"a", "b"}));
    }

    TEST(LinesOfTest, KeepsTheLastLineWithoutTrailingNewline)
    {
        EXPECT_EQ(lines_of("a\nb"), (std::vector<std::string>{"a", "b"}));
    }

    TEST(LinesOfTest, KeepsBlankLinesBetweenRecords)
    {
        EXPECT_EQ(lines_of("a\n\nb"), (std::vector<std::string>{"a", "", "b"}));
    }

    TEST(RecordLineCountTest, CountsEachRecordOnce)
    {
        EXPECT_EQ(record_line_count(kRecord + "\n" + kRecord + "\n"), 2U);
    }

    TEST(RecordLineCountTest, CountsAMarkerInsideAPayloadOnce)
    {
        EXPECT_EQ(record_line_count("2026-10-29T04:00:00Z 9 bytes=[ bytes=[ ]"), 1U);
    }

    TEST(RecordLineCountTest, IgnoresLinesWithoutTheMarker)
    {
        EXPECT_EQ(record_line_count("cabecera\n" + kRecord + "\n"), 1U);
    }

    TEST(RecordFieldsTest, ReadsTheTimestamp)
    {
        EXPECT_EQ(timestamp_of(kRecord), "2026-10-29T04:00:00Z");
    }

    TEST(RecordFieldsTest, ReturnsAnEmptyTimestampForAShortLine)
    {
        EXPECT_TRUE(timestamp_of("corto").empty());
    }

    TEST(RecordFieldsTest, ReadsThePayload)
    {
        EXPECT_EQ(payload_of(kRecord), "0x00, 0x01, 0x02");
    }

    TEST(RecordFieldsTest, ReturnsAnEmptyPayloadWithoutBrackets)
    {
        EXPECT_TRUE(payload_of("sin corchetes").empty());
    }

    TEST(CoutCaptureTest, CapturesStdCout)
    {
        CoutCapture capture;
        std::cout << "uno\ndos\n" << std::flush;

        EXPECT_EQ(capture.str(), "uno\ndos\n");
    }

    TEST(CoutCaptureTest, RestoresThePreviousBuffer)
    {
        {
            CoutCapture first;
            std::cout << "oculto\n" << std::flush;
        }

        CoutCapture second;
        std::cout << "visible\n" << std::flush;

        EXPECT_EQ(second.str(), "visible\n") << "the first capture must have restored std::cout";
    }

    TEST(CoutCaptureTest, DoesNotCaptureCStdioOutput)
    {
        CoutCapture capture;
        std::printf("esto va a la terminal, no al buffer\n");
        std::cout << "capturado\n" << std::flush;

        EXPECT_EQ(capture.str(), "capturado\n")
            << "a module printing with std::printf would look silent";
    }

    TEST(CollectingModuleTest, CollectsSubmittedPayloads)
    {
        CollectingModule sink;

        EXPECT_TRUE(sink.submit(ByteArray{1, 2}));
        EXPECT_TRUE(sink.submit(ByteArray{3}));

        EXPECT_EQ(sink.size(), 2U);
        EXPECT_EQ(sink.payloads(), (std::vector<ByteArray>{{1, 2}, {3}}));
    }

    TEST(CollectingModuleTest, WaitForAtLeastReturnsOnceTheCountArrives)
    {
        CollectingModule sink;
        sink.submit(ByteArray{1});

        EXPECT_TRUE(sink.waitForAtLeast(1, 100ms));
    }

    TEST(CollectingModuleTest, WaitForAtLeastTimesOutWhenNothingArrives)
    {
        CollectingModule sink;

        EXPECT_FALSE(sink.waitForAtLeast(1, 20ms));
    }

    TEST(CollectingModuleTest, StopReleasesTheWaitWithoutClaimingTheCount)
    {
        CollectingModule sink;
        sink.submit(ByteArray{1});

        std::promise<void> entered;
        std::promise<bool> reached;
        std::jthread waiter{[&sink, &entered, &reached] {
            entered.set_value();
            reached.set_value(sink.waitForAtLeast(5, 5s));
        }};

        entered.get_future().wait();
        sink.stop();

        auto result = reached.get_future();

        ASSERT_EQ(result.wait_for(1s), std::future_status::ready)
            << "stop must release the wait instead of burning the whole timeout";
        EXPECT_FALSE(result.get()) << "the module never received five payloads";
    }

    TEST(CollectingModuleTest, RejectsPayloadsAfterStop)
    {
        CollectingModule sink;
        sink.stop();

        EXPECT_FALSE(sink.submit(ByteArray{1}));
        EXPECT_EQ(sink.size(), 0U);
    }
}
