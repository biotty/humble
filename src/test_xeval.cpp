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
    auto r = xeval(x, env);
    ASSERT_EQ(3, get<VarNum>(*r).i);
}

