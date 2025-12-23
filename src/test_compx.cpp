#include "compx.hpp"
#include "debug.hpp"
#include "except.hpp"
#include "gtest/gtest.h"

using namespace humble;
using namespace std;

TEST(LexEnv, rewrite)
{
    std::vector<int> p = {5, 6};
    std::vector<int> c = {8};
    std::vector<int> n = {4, 5, 6, 1, 8};
    LexEnv le(p, c);
    ASSERT_EQ((LexArgs{{3, 0, 1, 4, 2}}),
            le.rewrite_names(n));
}

TEST(compx, unbound)
{
    string s = "((a (1 b)))";
    Names n = { "c", "d" };
    Macros m;
    try {
        auto t = compx(s, n, m, { 3 });
        FAIL();
    } catch (SrcError & e) {
        EXPECT_STREQ("unbound,\nline 1: a\n", e.what());
    }
    try {
        auto t = compx(s, n, m, { 2 });
        FAIL();
    } catch (SrcError & e) {
        EXPECT_STREQ("unbound,\nline 1: b\n", e.what());
    }
    try {
        auto t = compx(s, n, m, {});
        FAIL();
    } catch (SrcError & e) {
        EXPECT_STREQ("unbound,\nline 1: a\nline 1: b\n", e.what());
    }
    compx_dispose();
}

