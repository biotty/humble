#include "api.hpp"
#include "parse.hpp"
#include "gtest/gtest.h"

using namespace humble;
using namespace std;

TEST(parse, empty)
{
    Names m;
    ASSERT_EQ(0, parse("", m).v.size());
}

TEST(parse, flat)
{
    Names m;
    ASSERT_EQ(2, parse("0 1", m).v.size());
}

TEST(parse, beg_end)
{
    Names m;
    auto w = parse("()", m);
    ASSERT_EQ(1, w.v.size());
    ASSERT_EQ(0, get<LexForm>(w.v[0]).v.size());
}

TEST(parse, beg_end_mismatch)
{
    Names m;
    ASSERT_THROW(parse("(]", m), SrcError);
}

TEST(parse, beg_end_unmatch)
{
    Names m;
    ASSERT_THROW(parse("{", m), SrcError);
    ASSERT_THROW(parse("}", m), SrcError);
}

TEST(parse, quote)
{
    Names m = { "a", "b", "c", "d" };
    auto w = parse("'d", m);
    ASSERT_EQ(1, w.v.size());
    auto f = get<LexForm>(w.v[0]);
    ASSERT_EQ(2, f.v.size());
    ASSERT_EQ(3, get<LexNam>(f.v[1]).h);
}

TEST(parse, splice)
{
    Names m;
    auto w = parse("@a", m);
    ASSERT_EQ(1, w.v.size());
    auto s = get<LexSplice>(w.v[0]);
    ASSERT_TRUE(holds_alternative<LexNam>(s.v[0]));
}

