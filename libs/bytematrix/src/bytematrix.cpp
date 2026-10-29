#include "../include/bytematrix.h"

#include <string_view>

namespace bytematrix {
    std::string_view version() noexcept
    {
        return BYTEMATRIX_VERSION;
    }
}
