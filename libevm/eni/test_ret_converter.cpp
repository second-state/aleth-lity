#include <gtest/gtest.h>

#include "ret_converter.cpp"
#include "testutil.hpp"

namespace
{
using typeInfo_t = std::vector<Code>;

TEST(RetConverterTest, BoostIntAssumptions)
{
    boost::multiprecision::int256_t i = -1;
    char c = (unsigned)(0xff & i);
    EXPECT_EQ(c, '\xff');
}

TEST(RetConverterTest, NegInt)
{
    typeInfo_t f{INT};
    unsigned char eu[] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 133};
    std::string e(reinterpret_cast<const char*>(eu), sizeof(eu));
    EXPECT_EQ(ConvertReturnValue(f, "[-123]"), e);
}

TEST(RetConverterTest, String)
{
    typeInfo_t f{STRING};
    unsigned char eu[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 8, 45, 49, 50, 51, 97, 98, 99, 49, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    std::string e(reinterpret_cast<const char*>(eu), sizeof(eu));
    EXPECT_EQ(ConvertReturnValue(f, "[\"-123abc1\"]"), e);
}

TEST(RetConverterTest, EscapedString)
{
    typeInfo_t f{STRING};
    unsigned char eu[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 5, 45, 49, 50, 51, 34, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    std::string e(reinterpret_cast<const char*>(eu), sizeof(eu));
    EXPECT_EQ(ConvertReturnValue(f, "[\"-123\\\"\"]"), e);
}

TEST(RetConverterTest, ControlEscapedString)
{
    typeInfo_t f{STRING};
    unsigned char eu[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 7, 45, 49, 50, 51, 34, 10, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    std::string e(reinterpret_cast<const char*>(eu), sizeof(eu));
    EXPECT_EQ(ConvertReturnValue(f, "[\"-123\\\"\\n\\u0010\"]"), e);
}
TEST(RetConverterTest, UnicodeEscapedString)
{
    typeInfo_t f{STRING};
    EXPECT_THROW_MSG(
        ConvertReturnValue(f, "[\"-123\\\"\\n\\u7122\"]"), "unicode not implemented yet!");
}

TEST(RetConverterTest, FixArray)
{
    typeInfo_t f(34, (Code)0);
    f[0] = FIX_ARRAY_START;
    f[32] = (Code)1;
    f[33] = INT;
    unsigned char eu[] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 133};
    std::string e(reinterpret_cast<const char*>(eu), sizeof(eu));
    EXPECT_EQ(ConvertReturnValue(f, "[[-123]]"), e);
}

TEST(RetConverterTest, Error1)
{
    typeInfo_t f{INT};
    EXPECT_THROW_MSG(ConvertReturnValue(f, "-123"), "expected '[', found '-'");
}

TEST(RetConverterTest, Error2)
{
    typeInfo_t f{UINT};
    EXPECT_THROW_MSG(ConvertReturnValue(f, "[-123]"), "expected uint, found '-'");
}

TEST(RetConverterTest, Error3)
{
    typeInfo_t f{STRING};
    EXPECT_THROW_MSG(ConvertReturnValue(f, "[-123]"), "expected '\"', found '-'");
}

TEST(RetConverterTest, ErrorInt)
{
    {
        typeInfo_t f{INT};
        EXPECT_THROW_MSG(ConvertReturnValue(f, "[-]"), "expected int, found '-'");
    }
    {
        typeInfo_t f{UINT};
        EXPECT_THROW_MSG(ConvertReturnValue(f, "[-123]"), "expected uint, found '-'");
    }
}

TEST(RetConverterTest, ErrorBool)
{
    {
        typeInfo_t f{BOOL};
        EXPECT_THROW_MSG(ConvertReturnValue(f, "[tree]"), "expected 'u', found 'e'");
    }
    {
        typeInfo_t f{BOOL};
        EXPECT_THROW_MSG(ConvertReturnValue(f, "[jizz]"), "expected boolean, found 'j'");
    }
}

TEST(RetConverterTest, ErrorFixArray)
{
    typeInfo_t f(34);
    f[0] = FIX_ARRAY_START;
    f[32] = (Code)4;
    f[33] = INT;
    EXPECT_THROW_MSG(ConvertReturnValue(f, "[[-123, 7122, a, 45]]"), "expected int, found 'a'");
}

}  // namespace
