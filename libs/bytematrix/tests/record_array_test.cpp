#include <bytematrix.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <regex>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

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
    using bytematrix::test::timestamp_of;

    TEST(RecordStoreTest, PrintsUtcIsoTimestampAndHexPayload)
    {
        std::string output;

        {
            CoutCapture capture;
            auto recorder = bytematrix::createRecorder();
            recorder->start();
            recorder->submit(ByteArray{0x00, 0x01, 0x02});
            recorder->stop();
            output = capture.str();
        }

        const std::vector<std::string> lines = lines_of(output);
        ASSERT_EQ(lines.size(), 1U);
        EXPECT_TRUE(std::regex_match(
            lines.front(),
            std::regex(R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z bytes=\[00 01 02\])")))
            << "unexpected line: " << lines.front();
    }

    TEST(RecordStoreTest, PrintsMultiDigitValuesInLowercaseHex)
    {
        std::string output;

        {
            CoutCapture capture;
            auto recorder = bytematrix::createRecorder();
            recorder->start();
            recorder->submit(ByteArray{0xAB, 0x0F, 0x10});
            recorder->stop();
            output = capture.str();
        }

        const std::vector<std::string> lines = lines_of(output);
        ASSERT_EQ(lines.size(), 1U);
        EXPECT_EQ(payload_of(lines.front()), "ab 0f 10");
    }

    TEST(RecordStoreTest, PrintsAnEmptyPayloadAsEmptyBrackets)
    {
        std::string output;

        {
            CoutCapture capture;
            auto recorder = bytematrix::createRecorder();
            recorder->start();
            recorder->submit(ByteArray{});
            recorder->stop();
            output = capture.str();
        }

        const std::vector<std::string> lines = lines_of(output);
        ASSERT_EQ(lines.size(), 1U);
        EXPECT_EQ(payload_of(lines.front()), "");
        EXPECT_NE(lines.front().find("bytes=[]"), std::string::npos) << lines.front();
    }

    TEST(RecordStoreTest, PrintsEveryRecordReceivedSoFarOnEachArrival)
    {
        std::string output;

        {
            CoutCapture capture;
            auto recorder = bytematrix::createRecorder();
            recorder->start();
            recorder->submit(ByteArray{0x01});
            recorder->submit(ByteArray{0x02});
            recorder->submit(ByteArray{0x03});
            recorder->stop();
            output = capture.str();
        }

        const std::vector<std::string> lines = lines_of(output);
        ASSERT_EQ(lines.size(), 6U) << "3 arrivals must print 1 + 2 + 3 records";

        EXPECT_EQ(payload_of(lines[0]), "01");

        EXPECT_EQ(payload_of(lines[1]), "01");
        EXPECT_EQ(payload_of(lines[2]), "02");

        EXPECT_EQ(payload_of(lines[3]), "01");
        EXPECT_EQ(payload_of(lines[4]), "02");
        EXPECT_EQ(payload_of(lines[5]), "03");
    }

    TEST(RecordStoreTest, RestoresTheStreamFormattingAfterPrinting)
    {
        std::string output;

        {
            CoutCapture capture;
            auto recorder = bytematrix::createRecorder();
            recorder->start();
            recorder->submit(ByteArray{0xFF});
            recorder->stop();

            std::cout << 255 << '\n';
            output = capture.str();
        }

        EXPECT_NE(output.find("255"), std::string::npos)
            << "the recorder left std::cout in hex mode: " << output;
    }

    TEST(RecordStoreTest, KeepsRecordsOrderedByUtcInstant)
    {
        std::string output;

        {
            CoutCapture capture;
            auto recorder = bytematrix::createRecorder();
            recorder->start();
            recorder->submit(ByteArray{0x01});

            std::this_thread::sleep_for(1100ms);
            recorder->submit(ByteArray{0x02});

            recorder->stop();
            output = capture.str();
        }

        const std::vector<std::string> lines = lines_of(output);
        ASSERT_EQ(lines.size(), 3U) << "2 arrivals must print 1 + 2 records";

        std::vector<std::string> instants;
        instants.reserve(lines.size());
        for (const std::string& line : lines)
        {
            instants.push_back(timestamp_of(line));
        }

        EXPECT_LT(instants.front(), instants.back())
            << "records one second apart must print different instants";
        EXPECT_TRUE(std::ranges::is_sorted(instants));
    }

    TEST(RecordStoreTest, DoesNotForwardToADownstreamModule)
    {
        CollectingModule sink;
        std::string output;

        {
            CoutCapture capture;
            auto recorder = bytematrix::createRecorder();
            recorder->setNext(&sink);
            recorder->start();
            recorder->submit(ByteArray{0x00, 0x01, 0x02});
            recorder->stop();
            output = capture.str();
        }

        EXPECT_EQ(sink.size(), 0U) << "module 3 is the end of the pipeline";
        EXPECT_EQ(lines_of(output).size(), 1U);
    }
}
