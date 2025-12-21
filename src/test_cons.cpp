#include "cons.hpp"
#include "debug.hpp"
#include "api.hpp"
#include "gtest/gtest.h"

using namespace humble;
using namespace std;

TEST(cons, from_list)
{
    vector<EnvEntry> x = {
        make_shared<Var>(VarNum{1}),
        make_shared<Var>(VarNum{2}),
    };
    auto r = Cons::from_list(x);
    ASSERT_EQ(1, get<VarNum>(*get<EnvEntry>(r.c->a)).i);
    ASSERT_EQ(Cons::last, &*get<ConsPtr>(r.c->d));
    ASSERT_EQ(nullptr, get<ConsPtr>(Cons::last->d));
}

