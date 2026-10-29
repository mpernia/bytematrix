#include "PipelineRunner.hpp"

#include <csignal>
#include <iostream>
#include <thread>

namespace {
    constexpr auto kPollInterval = std::chrono::milliseconds{100};

    using SignalHandler = void (*)(int);

    volatile std::sig_atomic_t g_interrupted = 0;

    extern "C" void handle_signal([[maybe_unused]] int signal_number)
    {
        g_interrupted = 1;
    }

    class SignalGuard
    {
    public:
        SignalGuard()
            : previousInt_{std::signal(SIGINT, handle_signal)},
              previousTerm_{std::signal(SIGTERM, handle_signal)}
        {}

        ~SignalGuard()
        {
            std::signal(SIGINT, previousInt_);
            std::signal(SIGTERM, previousTerm_);
        }

        SignalGuard(const SignalGuard&) = delete;
        SignalGuard& operator=(const SignalGuard&) = delete;

    private:
        SignalHandler previousInt_;
        SignalHandler previousTerm_;
    };
}

PipelineRunner::PipelineRunner()
    : recorder_(bytematrix::createRecorder()), filter_(bytematrix::createFilter()),
      generator_(bytematrix::createGenerator())
{
    generator_->setNext(filter_.get());
    filter_->setNext(recorder_.get());

    start();
}

PipelineRunner::~PipelineRunner()
{
    stop();
}

void PipelineRunner::start()
{
    recorder_->start();
    filter_->start();
    generator_->start();
}

void PipelineRunner::stop()
{
    generator_->stop();
    filter_->stop();
    recorder_->stop();
}

void PipelineRunner::run(std::chrono::milliseconds duration)
{
    g_interrupted = 0;
    interrupted_ = false;

    const auto started = std::chrono::steady_clock::now();
    const auto deadline = started + duration;

    {
        const SignalGuard guard;

        while (std::chrono::steady_clock::now() < deadline)
        {
            if (g_interrupted != 0)
            {
                interrupted_ = true;
                break;
            }

            std::this_thread::sleep_for(kPollInterval);
        }
    }

    stop();

    elapsed_ = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() -
                                                                started);

    print();
}

void PipelineRunner::print() const
{
    std::cout << (interrupted_ ? "interrupted after " : "finished after ") << elapsed_.count()
              << " s\n"
              << std::flush;
}
