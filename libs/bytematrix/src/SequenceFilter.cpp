#include "SequenceFilter.hpp"

#include <algorithm>
#include <utility>

namespace bytematrix {
    SequenceFilter::SequenceFilter(ByteArray sequence)
        : QueuedModule(kInputBufferCapacity), sequence_(std::move(sequence))
    {}

    void SequenceFilter::setNext(IModule* next)
    {
        next_.store(next, std::memory_order_release);
    }

    void SequenceFilter::process(ByteArray data)
    {
        const bool found = !std::ranges::search(data, sequence_).empty();

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
