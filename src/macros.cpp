#include "macros.hpp"
#include "vars.hpp"
#include "cons.hpp"
#include "detail.hpp"
#include "except.hpp"

#include <iostream>

using namespace humble;
using namespace std;

namespace {

struct If : Macro {
    Lex operator()(LexForm & s) override
    {
        if (s.v.size() == 3) {
            s.v.push_back(LexVoid{});
        } else if (s.v.size() != 4) {
            throw SrcError("if-expression length");
        }
        return LexForm{{
            LexOp{ OP_COND },
                LexForm{{move(s.v[1]), move(s.v[2])}},
                LexForm{{LexBool{ true }, move(s.v[3])}}
        }};
    }
};

void with_name(Names & n, Macros & m, string name, unique_ptr<Macro> nm)
{
    int i = n.size();
    if (i != n.intern(name))
        throw CoreError("non-unique macro name");
    m[i] = move(nm);
}

} // ans

namespace humble {

Macros init_macros(Names & names, SrcOpener * opener)
{
    auto env_keys = GlobalEnv::instance().keys();
    auto m = qt_macros();

    (void)env_keys;
    (void)names;
    (void)opener;

    with_name(names, m, "if", make_unique<If>());

    return m;
}

Lex to_lex(EnvEntry a)
{
    if (holds_alternative<VarCons>(*a)) {
        auto b = get<VarCons>(*a).c->to_list_var();
        if (holds_alternative<VarList>(b))
            a = make_shared<Var>(get<VarList>(b));
        else
            a = make_shared<Var>(get<VarNonlist>(b));
    }
    return visit([](auto && q) -> Lex {
            using T = decay_t<decltype(q)>;
            if constexpr (is_same_v<T, VarList>) {
                vector<Lex> v;
                for (auto & y : q.v)
                    v.push_back(to_lex(y));
                return LexForm{ v };
            } else if constexpr (is_same_v<T, VarNonlist>) {
                auto b = make_shared<Var>(VarList{ q.v });
                return with_dot(get<LexForm>(to_lex(b)));
            } else if constexpr (is_same_v<T, VarBool>) {
                return LexBool{ q.b };
            } else if constexpr (is_same_v<T, VarNum>) {
                return LexNum{ q.i };
            } else if constexpr (is_same_v<T, VarString>) {
                return LexString{ q.s };
            } else if constexpr (is_same_v<T, VarNam>) {
                return LexNam{ q.h, 0 };
            } else if constexpr (is_same_v<T, VarUnquote>) {
                return *q.u;
            } else if constexpr (is_same_v<T, VarSplice>) {
                throw RunError("splice to lex");
            } else if constexpr (is_same_v<T, VarFunOps>) {
                throw RunError("fun to lex");
            } else if constexpr (is_same_v<T, VarFunHost>) {
                throw RunError("host-fun to lex");
            } else if constexpr (is_same_v<T, VarApply>) {
                throw RunError("latent-apply to lex");
            } else if constexpr (is_same_v<T, VarVoid>) {
                return LexVoid{};
            } else {
                throw CoreError("to lex not handled");
            }
    }, *a);
}

EnvEntry from_lex(const Lex & x)
{
    (void)x;
    return make_shared<Var>(VarVoid{});
}

} // ns

