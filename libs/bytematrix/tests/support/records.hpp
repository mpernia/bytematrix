#ifndef BYTEMATRIX_TESTS_SUPPORT_RECORDS_HPP
#define BYTEMATRIX_TESTS_SUPPORT_RECORDS_HPP

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace bytematrix::test {

    inline constexpr std::string_view kRecordMarker = " bytes=[";

    inline constexpr std::size_t kTimestampLength = 20;

    inline std::vector<std::string> lines_of(const std::string& output)
    {
        std::vector<std::string> lines;
        std::size_t begin = 0;

        while (begin < output.size())
        {
            const std::size_t end = output.find('\n', begin);
            if (end == std::string::npos)
            {
                lines.push_back(output.substr(begin));
                break;
            }

            lines.push_back(output.substr(begin, end - begin));
            begin = end + 1;
        }

        return lines;
    }

    inline std::size_t record_line_count(const std::string& output)
    {
        std::size_t count = 0;

        for (const auto& line : lines_of(output))
        {
            if (line.find(kRecordMarker) != std::string::npos)
            {
                ++count;
            }
        }

        return count;
    }

    inline std::string timestamp_of(const std::string& line)
    {
        if (line.size() < kTimestampLength)
        {
            return {};
        }

        return line.substr(0, kTimestampLength);
    }

    inline std::string payload_of(const std::string& line)
    {
        const std::size_t open = line.find('[');
        const std::size_t close = line.find(']');

        if (open == std::string::npos || close == std::string::npos || close < open)
        {
            return {};
        }

        return line.substr(open + 1, close - open - 1);
    }
}

#endif // BYTEMATRIX_TESTS_SUPPORT_RECORDS_HPP
