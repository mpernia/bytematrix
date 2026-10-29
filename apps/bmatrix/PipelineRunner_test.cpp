#include <chrono>
#include <csignal>
#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include "PipelineRunner.hpp"

namespace {
    using namespace std::chrono_literals;

    // Takes std::cout for the scope. Only safe around a run: the pipeline writes
    // from its workers while it runs, and every test reads the text once the
    // runner is gone and its threads are joined.
    class CoutCapture
    {
    public:
        CoutCapture() : previous_{std::cout.rdbuf(buffer_.rdbuf())} {}

        ~CoutCapture()
        {
            std::cout.rdbuf(previous_);
        }

        CoutCapture(const CoutCapture&) = delete;
        CoutCapture& operator=(const CoutCapture&) = delete;

        [[nodiscard]] std::string str() const
        {
            return buffer_.str();
        }

    private:
        std::ostringstream buffer_;
        std::streambuf* previous_;
    };

    // A four-byte sequence that random payloads of 1..100 bytes almost never
    // contain, so it almost never forwards anything during a test run.
    const bytematrix::ByteArray kRareSequence{0xDE, 0xAD, 0xBE, 0xEF};

    // Raises SIGINT once the run is already polling, so the default action can
    // never fire before the runner installs its handler.
    std::thread interrupt_after(std::chrono::milliseconds delay)
    {
        return std::thread{[delay] {
            std::this_thread::sleep_for(delay);
            std::raise(SIGINT);
        }};
    }

    TEST(PipelineRunnerTest, RunReportsTheElapsedOfTheRun)
    {
        CoutCapture capture;

        {
            PipelineRunner runner{kRareSequence};
            runner.run(1100ms);
        }

        // The recorder may have printed records while it ran, so the assertion
        // is that the summary comes last: nothing wrote after it.
        EXPECT_TRUE(capture.str().ends_with("finished after 1 s\n")) << capture.str();
    }

    TEST(PipelineRunnerTest, RunReturnsOnceTheDeadlinePasses)
    {
        CoutCapture capture;
        const auto started = std::chrono::steady_clock::now();

        {
            PipelineRunner runner{kRareSequence};
            runner.run(300ms);
        }

        const auto elapsed = std::chrono::steady_clock::now() - started;
        EXPECT_GE(elapsed, 300ms) << "run must wait for the whole deadline";
        EXPECT_LT(elapsed, 3s) << "run must not overshoot the deadline";
    }

    TEST(PipelineRunnerTest, RunStopsAsSoonAsSigintArrives)
    {
        CoutCapture capture;
        const auto started = std::chrono::steady_clock::now();

        {
            PipelineRunner runner{kRareSequence};
            std::thread interrupter = interrupt_after(300ms);

            runner.run(10s);
            interrupter.join();
        }

        EXPECT_LT(std::chrono::steady_clock::now() - started, 3s)
            << "SIGINT must cut the 10 s deadline short";
        EXPECT_EQ(capture.str(), "interrupted after 0 s\n");
    }

    TEST(PipelineRunnerTest, RunRestoresThePreviousSignalHandler)
    {
        CoutCapture capture;

        {
            PipelineRunner runner{kRareSequence};
            runner.run(0ms);
        }

        // signal() both queries and sets, so put back whatever it reports.
        void (*previous)(int) = std::signal(SIGINT, SIG_DFL);
        std::signal(SIGINT, previous);

        EXPECT_EQ(previous, SIG_DFL) << "a run must not leave its handler installed";
    }

    TEST(PipelineRunnerTest, ASecondRunIsNotInterruptedByTheFirstSigint)
    {
        CoutCapture capture;

        {
            PipelineRunner runner{kRareSequence};
            std::thread interrupter = interrupt_after(300ms);

            runner.run(10s);
            interrupter.join();

            runner.run(100ms);
        }

        EXPECT_EQ(capture.str(), "interrupted after 0 s\nfinished after 0 s\n")
            << "the interrupt flag must not leak into the next run";
    }
    TEST(PipelineRunnerTest, RejectsAnEmptySequence)
    {
        EXPECT_THROW(static_cast<void>(PipelineRunner{bytematrix::ByteArray{}}),
                     std::invalid_argument)
            << "the library refuses an empty sequence and the runner must not swallow it";
    }
}
