#include <gtest/gtest.h>
#include <string_view>

#include <bytematrix.h>

// EXPECTED_VERSION is injected by CMake from the library's project() version.
TEST(Version, ReportsTheVersionDeclaredInCMake)
{
    EXPECT_EQ(bytematrix::version(), std::string_view{EXPECTED_VERSION});
}

TEST(Version, IsNotEmpty)
{
    EXPECT_FALSE(bytematrix::version().empty());
}
