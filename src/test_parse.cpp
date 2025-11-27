#include "api.hpp"
#include "parse.hpp"
#include "gtest/gtest.h"

using namespace humble;
using namespace std;

TEST(parse, empty)
{
    Names m;
    ASSERT_EQ(0, parse_i("", m).v.size());
}

TEST(parse, flat)
{
    Names m;
    ASSERT_EQ(2, parse_i("0 1", m).v.size());
}

TEST(parse, beg_end)
{
    Names m;
    auto w = parse_i("()", m);
    ASSERT_EQ(1, w.v.size());
    ASSERT_EQ(0, get<LexForm>(w.v[0]).v.size());
}

TEST(parse, beg_end_mismatch)
{
    Names m;
    ASSERT_THROW(parse_i("(]", m), SrcError);
}

TEST(parse, beg_end_unmatch)
{
    Names m;
    ASSERT_THROW(parse_i("{", m), SrcError);
    ASSERT_THROW(parse_i("}", m), SrcError);
}

TEST(parse, quote)
{
    Names m = { "a", "b", "c", "d" };
    auto w = parse_i("'d", m);
    ASSERT_EQ(1, w.v.size());
    auto f = get<LexForm>(w.v[0]);
    ASSERT_EQ(2, f.v.size());
    ASSERT_EQ(3, get<LexName>(f.v[1]).h);
}

TEST(parse, splice)
{
    Names m;
    auto w = parse_i("@a", m);
    ASSERT_EQ(1, w.v.size());
    auto s = get<LexSplice>(w.v[0]);
    ASSERT_TRUE(holds_alternative<LexName>(s.v[0]));
}

