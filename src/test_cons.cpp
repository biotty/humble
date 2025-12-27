#include "cons.hpp"
#include "debug.hpp"
#include "except.hpp"
#include "gtest/gtest.h"

using namespace humble;
using namespace std;

struct ConsTest : testing::Test {
    EnvEntry s;
    EnvEntry t;
    void SetUp() override {
        s = make_shared<Var>(VarNum{1});
        t = make_shared<Var>(VarNum{2});
    }
};

TEST_F(ConsTest, from_list)
{
    vector<EnvEntry> x = { s, t };
    auto r = Cons::from_list(x);
    ASSERT_EQ(1, get<VarNum>(*r.c->a).i);
    ASSERT_EQ(Cons::last, &*get<ConsPtr>(r.c->d));
    ASSERT_EQ(nullptr, get<ConsPtr>(Cons::last->d));
}

TEST_F(ConsTest, xcopy)
{
    auto c = make_shared<Cons>(s,
            make_shared<Cons>(t, ConsPtr{}));
    auto b = c->xcopy(0);
    ASSERT_EQ(c->a, b.c->a);
    ASSERT_NE(c->d, b.c->d);
    ASSERT_EQ(Cons::last, &*get<ConsPtr>(b.c->d));
}

TEST_F(ConsTest, length)
{
    auto c = make_shared<Cons>(s,
            make_shared<Cons>(t, ConsPtr{}));
    ASSERT_EQ(2, c->length());
}

TEST_F(ConsTest, to_var_list)
{
    {
        auto c = make_shared<Cons>(s,
                make_shared<Cons>(t, ConsPtr{}));
        ASSERT_EQ((vector<EnvEntry>{s, t}), get<VarList>(c->to_list_var()).v);
    }
    {
        auto c = make_shared<Cons>(s,
                make_shared<Cons>(t, s));
        ASSERT_EQ((vector<EnvEntry>{s, t, s}), get<VarNonlist>(c->to_list_var()).v);
    }
}
