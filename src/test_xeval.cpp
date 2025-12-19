#include "xeval.hpp"
#include "compx.hpp"
#include "cons.hpp"
#include "debug.hpp"
#include "api.hpp"
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

EnvEntry echo1(std::span<EnvEntry> a)
{
    if (a.empty()) throw RunError("echo1");
    return a[0];
}

TEST_F(EnvTest, xeval_fun_host)
{
    env.set(2, make_shared<Var>(VarFunHost{echo1}));
    Lex x = LexForm{{LexNam{2, 0}, LexNum{9}}};
    auto r = run(x, env);
    ASSERT_EQ(9, get<VarNum>(*r).i);
}

TEST_F(EnvTest, xeval_fun_ops)
{
    LexEnv le({}, {});
    Lex x = LexForm{{LexForm{{LexOp{OP_LAMBDA},
        &le, LexArgs{}, LexNum{9}}}}};
    // cout << &get<LexForm>(get<LexForm>(x).v.at(0)).v.at(3) << " test\n";
    // cout << get<LexNum>(get<LexForm>(get<LexForm>(x).v.at(0)).v.at(3)).i << " test\n";
    auto r = run(x, env);
    ASSERT_EQ(9, get<VarNum>(*r).i);
}

TEST_F(EnvTest, xeval_tco)
{
    LexEnv le({}, {});
    Lex x = LexForm{{LexForm{{LexOp{OP_LAMBDA},
        &le, LexArgs{},
        LexForm{{LexForm{{LexOp{OP_LAMBDA},
            &le, LexArgs{}, LexNum{9}}}}}
    }}}};
    auto r = run(x, env);
    ASSERT_EQ(9, get<VarNum>(*r).i);
}

TEST_F(EnvTest, xeval_nonlist_cat)
{
    Lex x = LexNonlist{{LexNum{0},
        LexList{{LexNum{1}, LexNum{2}}}
    }};
    auto r = run(x, env);
    ASSERT_EQ(3, get<VarList>(*r).v.size());
}

EnvEntry cons1(std::span<EnvEntry> a)
{
    if (a.size() != 1) throw RunError("cons1");
    return make_shared<Var>(VarCons{
            make_shared<Cons>(a[0], ConsPtr{})});
}

TEST_F(EnvTest, xeval_cons_cat)
{
    env.set(2, make_shared<Var>(VarFunHost{cons1}));
    Lex x = LexNonlist{{LexNum{0},
        LexForm{{LexNam{2, 0}, LexNum{1}}}
    }};
    auto r = run(x, env);
    ASSERT_TRUE(holds_alternative<VarCons>(*r));
}

