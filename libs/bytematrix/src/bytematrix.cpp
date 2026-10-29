#include "bytematrix.h"

#include <stdexcept>
#include <string_view>
#include <utility>

#include "ByteArrayRecorder.hpp"
#include "ByteGenerator.hpp"
#include "SequenceFilter.hpp"

namespace bytematrix {
    std::string_view version() noexcept
    {
        return BYTEMATRIX_VERSION;
    }

    std::unique_ptr<IModule> createGenerator()
    {
        return std::make_unique<ByteGenerator>();
    }

    std::unique_ptr<IModule> createFilter(ByteArray sequence)
    {
        if (sequence.empty())
        {
            throw std::invalid_argument{"the sequence to search for must not be empty"};
        }

        return std::make_unique<SequenceFilter>(std::move(sequence));
    }

    std::unique_ptr<IModule> createRecorder()
    {
        return std::make_unique<ByteArrayRecorder>();
    }
}
