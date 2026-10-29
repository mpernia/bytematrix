#include <chrono>
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <initializer_list>
#include <string>
#include <thread>
#include <vector>

#include "support/cout_capture.hpp"
#include "support/records.hpp"
#include <bytematrix.h>

namespace {
    using namespace std::chrono_literals;

    using bytematrix::ByteArray;

    ByteArray bytes(std::initializer_list<std::uint8_t> values)
    {
        return ByteArray{values};
    }

    TEST(PipelineTest, FilterForwardsOnlyMatchingPayloadsToTheRecorder)
    {
        std::string output;

        {
            bytematrix::test::CoutCapture capture;
            auto filter = bytematrix::createFilter();
            auto recorder = bytematrix::createRecorder();
            filter->setNext(recorder.get());

            filter->start();
            recorder->start();

            filter->submit(bytes({0xAA, 0x00, 0x01, 0x02})); // matches
            filter->submit(bytes({0xCC, 0xDD}));             // discarded
            filter->submit(bytes({0x00, 0x01, 0x02}));       // matches

            filter->stop();
            recorder->stop();
            output = capture.str();
        }

        const std::vector<std::string> lines = bytematrix::test::lines_of(output);
        ASSERT_EQ(lines.size(), 3U) << "2 arrivals must print 1 + 2 records";
        EXPECT_EQ(bytematrix::test::payload_of(lines[0]), "aa 00 01 02");
        EXPECT_EQ(bytematrix::test::payload_of(lines[2]), "00 01 02");
    }

    TEST(PipelineTest, HandlesConcurrentProducersThroughTheWholeChain)
    {
        constexpr std::size_t kProducers = 4;
        constexpr std::size_t kPayloadsPerProducer = 5;
        constexpr std::size_t kArrivals = kProducers * kPayloadsPerProducer;

        std::string output;

        {
            bytematrix::test::CoutCapture capture;
            auto filter = bytematrix::createFilter();
            auto recorder = bytematrix::createRecorder();
            filter->setNext(recorder.get());

            filter->start();
            recorder->start();

            std::vector<std::thread> producers;
            producers.reserve(kProducers);

            for (std::size_t producer = 0; producer < kProducers; ++producer)
            {
                producers.emplace_back([&filter, producer] {
                    for (std::size_t index = 0; index < kPayloadsPerProducer; ++index)
                    {
                        filter->submit(bytes({0x00, 0x01, 0x02, static_cast<std::uint8_t>(producer),
                                              static_cast<std::uint8_t>(index)}));
                    }
                });
            }

            for (std::thread& producer : producers)
            {
                producer.join();
            }

            filter->stop();
            recorder->stop();
            output = capture.str();
        }

        // Every arrival reprints the whole history: 1 + 2 + ... + 20 lines.
        const std::size_t expectedLines = kArrivals * (kArrivals + 1) / 2;
        EXPECT_EQ(bytematrix::test::record_line_count(output), expectedLines);

        for (const std::string& line : bytematrix::test::lines_of(output))
        {
            EXPECT_NE(bytematrix::test::payload_of(line).find("00 01 02"), std::string::npos)
                << "a non-matching payload reached the recorder: " << line;
        }
    }

    TEST(PipelineTest, RunsTheGeneratorChainWithoutLosingThePatternInvariant)
    {
        std::string output;

        {
            bytematrix::test::CoutCapture capture;
            auto generator = bytematrix::createGenerator();
            auto filter = bytematrix::createFilter();
            auto recorder = bytematrix::createRecorder();

            generator->setNext(filter.get());
            filter->setNext(recorder.get());

            generator->start();
            filter->start();
            recorder->start();

            std::this_thread::sleep_for(300ms);

            generator->stop();
            filter->stop();
            recorder->stop();
            output = capture.str();
        }

        // The statement's pattern is rare, so there may be no record at all;
        // whenever one appears it must contain it.
        for (const std::string& line : bytematrix::test::lines_of(output))
        {
            EXPECT_NE(bytematrix::test::payload_of(line).find("00 01 02"), std::string::npos)
                << "unexpected record: " << line;
        }

        SUCCEED() << "the three threads start, run together and stop without crashing";
    }
}
