#include "compx.hpp"
#include "debug.hpp"
#include "except.hpp"
#include "gtest/gtest.h"

using namespace humble;
using namespace std;

#ifdef DEBUG
#define HUMBLE_EXCEPT_M(E, M) EXPECT_TRUE(string(E.what()).starts_with(M "\n"))
#else
#define HUMBLE_EXCEPT_M(E, M) EXPECT_STREQ(M, E.what())
#endif

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
    Names n = init_names();
    n.intern("c");
    auto i = n.intern("d");
    string s = "((a (1 b)))";
    auto i_a = ++i;
    auto i_b = ++i;
    Macros m;
    try {
        auto t = compx(s, n, m, { i_b });
        FAIL();
    } catch (SrcError & e) {
        HUMBLE_EXCEPT_M(e, "unbound,\nline 1: a\n");
    }
    try {
        auto t = compx(s, n, m, { i_a });
        FAIL();
    } catch (SrcError & e) {
        HUMBLE_EXCEPT_M(e, "unbound,\nline 1: b\n");
    }
    try {
        auto t = compx(s, n, m, {});
        FAIL();
    } catch (SrcError & e) {
        HUMBLE_EXCEPT_M(e, "unbound,\nline 1: a\nline 1: b\n");
    }
    compx_dispose();
}

