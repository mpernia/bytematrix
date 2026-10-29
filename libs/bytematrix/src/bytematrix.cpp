#include "bytematrix.h"

#include <string_view>

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

    std::unique_ptr<IModule> createFilter()
    {
        return std::make_unique<SequenceFilter>();
    }

    std::unique_ptr<IModule> createRecorder()
    {
        return std::make_unique<ByteArrayRecorder>();
    }
}
