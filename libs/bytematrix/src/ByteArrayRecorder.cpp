#include "ByteArrayRecorder.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <utility>

namespace bytematrix {
    ByteArrayRecorder::ByteArrayRecorder() : QueuedModule(kInputBufferCapacity) {}

    void ByteArrayRecorder::setNext([[maybe_unused]] IModule* next) {}

    void ByteArrayRecorder::process(ByteArray data)
    {
        records_.push_back(
            Record{.timestamp = std::chrono::system_clock::now(), .data = std::move(data)});

        if (records_.size() > 1 &&
            records_[records_.size() - 2].timestamp > records_.back().timestamp)
        {
            std::ranges::stable_sort(records_, [](const Record& left, const Record& right) {
                return left.timestamp < right.timestamp;
            });
        }

        printRecords();
    }

    void ByteArrayRecorder::printRecords() const
    {
        const std::ios_base::fmtflags flags = std::cout.flags();
        const char fill = std::cout.fill();

        for (const Record& record : records_)
        {
            const std::time_t timestamp = std::chrono::system_clock::to_time_t(record.timestamp);

            std::tm utcTime{};
            if (gmtime_r(&timestamp, &utcTime) == nullptr)
            {
                continue;
            }

            std::cout << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%SZ") << " bytes=[";

            for (std::size_t index = 0; index < record.data.size(); ++index)
            {
                if (index != 0)
                {
                    std::cout << ' ';
                }

                std::cout << std::hex << std::setw(2) << std::setfill('0')
                          << static_cast<unsigned int>(record.data[index]);
            }

            std::cout << "]\n";
        }

        std::cout.flags(flags);
        std::cout.fill(fill);
        std::cout << std::flush;
    }
}
