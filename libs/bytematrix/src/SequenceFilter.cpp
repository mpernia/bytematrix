#include "SequenceFilter.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace bytematrix {
    namespace {
        constexpr std::array<std::uint8_t, 3> kSequence{0x00, 0x01, 0x02};
    }

    SequenceFilter::SequenceFilter() : QueuedModule(kInputBufferCapacity) {}

    void SequenceFilter::setNext(IModule* next)
    {
        next_.store(next, std::memory_order_release);
    }

    void SequenceFilter::process(ByteArray data)
    {
        const bool found = !std::ranges::search(data, kSequence).empty();

        if (!found)
        {
            return;
        }

        IModule* next = next_.load(std::memory_order_acquire);

        if (next != nullptr)
        {
            next->submit(std::move(data));
        }
    }
}
