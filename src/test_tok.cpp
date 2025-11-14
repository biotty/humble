#include "api.hpp"
#include "tok.hpp"
#include "gtest/gtest.h"

using namespace humble;

TEST(spaces, null_arg)
{
    ASSERT_THROW(spaces(nullptr), CoreError);
}

TEST(spaces, empty)
{
    auto s = "";
    ASSERT_EQ(s, spaces(s));
}

TEST(spaces, skip_one)
{
    auto s = " a";
    ASSERT_EQ(s + 1, spaces(s));
}

TEST(spaces, skip_some)
{
    linenumber = 0;
    auto s = "\t\n\r\v\ta";
    ASSERT_EQ(s + 5, spaces(s));
    EXPECT_EQ(1, linenumber);
}

TEST(spaces, skip_comment)
{
    linenumber = 0;
    auto s = " ;b\na";
    ASSERT_EQ(s + 4, spaces(s));
    EXPECT_EQ(1, linenumber);
}

TEST(spaces, skip_multiline_comment)
{
    linenumber = 0;
    auto s = " #|b\n\n|#a";
    ASSERT_EQ(s + 8, spaces(s));
    EXPECT_EQ(2, linenumber);
}

TEST(spaces, err_multiline_not_ended)
{
    auto s = "#|a";
    ASSERT_THROW(spaces(s), SrcError);
}

