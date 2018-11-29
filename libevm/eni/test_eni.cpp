#include <gtest/gtest.h>

#include "ENI.hpp"


namespace
{
TEST(ENITest, Reverse)
{
    ENI eni;
    eni.InitENI("reverse", R"(["abc123"])");
    EXPECT_EQ(R"(["321cba"])", eni.ExecuteENI());
}

}  // namespace
