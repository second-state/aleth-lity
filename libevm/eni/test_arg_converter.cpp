#include <gtest/gtest.h>

#include "arg_converter.cpp"
#include "testutil.hpp"

namespace
{
TEST(ArgConverterTest, PosInt)
{
    std::vector<Code> f{INT};
    std::string d(70, 0);
    d[31] = 72;
    EXPECT_EQ(ConvertArguments(f, d), "[72]");
}

TEST(ArgConverterTest, Bool)
{
    std::vector<Code> f{BOOL};
    std::string d(32, 0);
    EXPECT_EQ(ConvertArguments(f, d), "[false]");

    for (int i = 0; i < 32; ++i)
    {
        std::string d(32, 0);
        d[i] = 72;
        EXPECT_EQ(ConvertArguments(f, d), "[true]");
    }
}

TEST(ArgConverterTest, IntBool)
{
    std::vector<Code> f{INT, BOOL};
    std::string d(70, 0);
    d[31] = 72;
    EXPECT_EQ(ConvertArguments(f, d), "[72,false]");
}

TEST(ArgConverterTest, NegInt)
{
    std::vector<Code> f{INT};
    std::string d(70, (char)255);
    EXPECT_EQ(ConvertArguments(f, d), "[-1]");

    d[31] = '\x85';
    EXPECT_EQ(ConvertArguments(f, d), "[-123]");
}

TEST(ArgConverterTest, String)
{
    std::vector<Code> f{STRING, STRING};
    std::string d(160, 0);
    d[31] = 3;
    d.replace(32, 4, "abcd");
    d[95] = 50;
    d.replace(96, 52, "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");
    EXPECT_EQ(
        ConvertArguments(f, d), "[\"abc\",\"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwx\"]");
}


TEST(ArgConverterTest, EscapedString)
{
    std::vector<Code> f{STRING};
    std::string d(64, 0);
    d[31] = 7;
    const char* strA = "abc\"d\\e";
    std::copy(strA, strA + 9, d.begin() + 32);

    EXPECT_EQ(ConvertArguments(f, d), "[\"abc\\\"d\\\\e\"]");
}

TEST(ArgConverterTest, ControlEscapedString)
{
    std::vector<Code> f{STRING};
    std::string d(64, 0);
    d[31] = 9;
    const char strA[] = "abc\"d\\\b\0e";
    std::copy(strA, strA + 9, d.begin() + 32);

    EXPECT_EQ(ConvertArguments(f, d), "[\"abc\\\"d\\\\\\u0008\\u0000e\"]");
}

TEST(ArgConverterTest, ErrorEncoding1)
{
    std::vector<Code> f{(Code)155};
    std::string d(70, 0);
    for (int i = 0; i < 32; ++i)
    {
        d[i] = '\xff';
    }

    EXPECT_THROW_MSG(
        ConvertArguments(f, d), "encoding error - unknown or not implemented type: 155");
}

TEST(ArgConverterTest, ErrorEncoding2)
{
    std::vector<Code> f{STRUCT_START, INT};
    std::string d(70, 0);
    for (int i = 0; i < 32; ++i)
    {
        d[i] = '\xff';
    }

    EXPECT_THROW_MSG(ConvertArguments(f, d), "typeInfo index out of range");
}

}  // namespace
