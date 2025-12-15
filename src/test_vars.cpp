#include "vars.hpp"
#include "gtest/gtest.h"

using namespace humble;
using namespace std;

struct EnvTest : testing::Test {
    GlobalEnv env{GlobalEnv::create_t{}};
};

TEST_F(EnvTest, lookup)
{
    EnvEntry p = make_shared<Var>(VarNum{1});
    env.set(5, p);
    ASSERT_EQ(p, env.get(5));
    ASSERT_EQ(nullptr, env.get(6));
}

TEST_F(EnvTest, overlay)
{
    EnvEntry p = make_shared<Var>(VarNum{1});
    EnvEntry q = make_shared<Var>(VarNum{2});
    env.set(5, p);
    env.set(6, q);
    OverlayEnv w(env);
    ASSERT_EQ(p, w.get(5));
    ASSERT_EQ(nullptr, w.get(7));
    w.set(7, q);
    ASSERT_EQ(q, w.get(7));
}

TEST(localenv, setget)
{
    EnvEntry p = make_shared<Var>(VarNum{1});
    FunEnv le{nullptr, nullptr};
    le.set(1, p);
    ASSERT_EQ(p, le.get(1));
    ASSERT_EQ(nullptr, le.get(0));
    // comment: causes heap-buf-overfl (asan detects)
    // le.get(2);
}

