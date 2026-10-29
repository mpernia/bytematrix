#include <charconv>
#include <chrono>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <system_error>

#include "PipelineRunner.hpp"
#include "bytematrix.h"

namespace {
    constexpr int kDefaultSeconds = 100;

    int parse_seconds(const char* text)
    {
        const std::string_view input{text};
        int seconds = 0;
        const auto [end, error] =
            std::from_chars(input.data(), input.data() + input.size(), seconds);

        if (error != std::errc{} || end != input.data() + input.size())
        {
            throw std::invalid_argument{"duration must be a whole number of seconds"};
        }

        if (seconds < 0)
        {
            throw std::out_of_range{"duration must be non-negative"};
        }

        return seconds;
    }
}

int main(int argc, char* argv[])
{
    int run_seconds = kDefaultSeconds;

    if (argc > 1)
    {
        try
        {
            run_seconds = parse_seconds(argv[1]);
        }
        catch (const std::exception& error)
        {
            std::cerr << "Invalid duration: " << error.what() << '\n'
                      << "Usage: " << argv[0] << " [run-seconds]\n";
            return 1;
        }
    }

    std::cout << "demo_libbytematrix | libbytematrix " << bytematrix::version() << " | "
              << run_seconds << " s (Ctrl-C stops early)\n"
              << std::flush;

    PipelineRunner runner;
    runner.run(std::chrono::seconds{run_seconds});

    return 0;
}
