#include "../include/bytematrix.h"
#include "ByteGenerator.hpp"

#include <string_view>

namespace bytematrix {
    std::string_view version() noexcept
    {
        return BYTEMATRIX_VERSION;
    }

    std::unique_ptr<IModule> createGenerator()
    {
        return std::make_unique<ByteGenerator>();
    }
}
