#include <charconv>
#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include "PipelineRunner.hpp"
#include "bytematrix.h"

namespace {
    constexpr int kDefaultSeconds = 100;

    // What the demo looks for when the command line does not say otherwise.
    const bytematrix::ByteArray kFallbackSequence{0x00, 0x01, 0x02};

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

    // Comma-separated hex bytes, each one or two digits with an optional 0x prefix:
    // "00,01,02" or "0x0,0x1,0x2". A packed form like "000102" is rejected on
    // purpose, because it cannot be told apart from six single-digit bytes.
    bytematrix::ByteArray parse_sequence(const std::string_view text)
    {
        constexpr std::string_view kExpected{
            "expected comma-separated hex bytes, for example 00,01,02"};

        bytematrix::ByteArray sequence;

        std::size_t start = 0;
        while (true)
        {
            const std::size_t comma = text.find(',', start);
            const std::size_t end = comma == std::string_view::npos ? text.size() : comma;

            std::string_view token = text.substr(start, end - start);
            if (token.starts_with("0x") || token.starts_with("0X"))
            {
                token.remove_prefix(2);
            }

            unsigned int value = 0;
            const auto [parsed, error] =
                std::from_chars(token.data(), token.data() + token.size(), value, 16);

            if (token.empty() || token.size() > 2 || error != std::errc{} ||
                parsed != token.data() + token.size() || value > 0xFFU)
            {
                throw std::invalid_argument{std::string{kExpected}};
            }

            sequence.push_back(static_cast<std::uint8_t>(value));

            if (comma == std::string_view::npos)
            {
                return sequence;
            }

            start = comma + 1;
        }
    }

    std::string describe(const bytematrix::ByteArray& sequence)
    {
        constexpr char kDigits[] = "0123456789abcdef";

        std::string text;
        for (const std::uint8_t byte : sequence)
        {
            if (!text.empty())
            {
                text += ' ';
            }

            text += kDigits[byte >> 4U];
            text += kDigits[byte & 0x0FU];
        }

        return text;
    }

    void print_usage(const char* program)
    {
        std::cerr << "Usage: " << program << " [run-seconds] [sequence]\n";
    }
}

int main(int argc, char* argv[])
{
    int run_seconds = kDefaultSeconds;
    bytematrix::ByteArray sequence = kFallbackSequence;

    if (argc > 1)
    {
        try
        {
            run_seconds = parse_seconds(argv[1]);
        }
        catch (const std::exception& error)
        {
            std::cerr << "Invalid duration: " << error.what() << '\n';
            print_usage(argv[0]);
            return 1;
        }
    }

    if (argc > 2)
    {
        try
        {
            sequence = parse_sequence(argv[2]);
        }
        catch (const std::exception& error)
        {
            std::cerr << "Invalid sequence: " << error.what() << '\n';
            print_usage(argv[0]);
            return 1;
        }
    }

    std::cout << "demo_libbytematrix | libbytematrix " << bytematrix::version() << " | "
              << run_seconds << " s | looking for " << describe(sequence)
              << " (Ctrl-C stops early)\n"
              << std::flush;

    PipelineRunner runner{std::move(sequence)};
    runner.run(std::chrono::seconds{run_seconds});

    return 0;
}
