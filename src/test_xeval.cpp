#include "xeval.hpp"
#include "gtest/gtest.h"

using namespace humble;
using namespace std;

struct EnvTest : testing::Test {
    GlobalEnv env{GlobalEnv::create_t{}};
};

TEST_F(EnvTest, xeval_num)
{
    Lex x = LexNum{3};
    auto r = run(x, env);
    ASSERT_EQ(3, get<VarNum>(*r).i);
}

TEST_F(EnvTest, xeval_splice)
{
    env.set(9, make_shared<Var>(VarSplice{{
                make_shared<Var>(VarBool{true})}}));
    Lex x = LexList{{LexNam{9, 0}}};
    auto r = run(x, env);
    auto & v = get<VarList>(*r).v;
    ASSERT_EQ(true, get<VarBool>(*v.at(0)).b);
}

