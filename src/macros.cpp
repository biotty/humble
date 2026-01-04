#include "macros.hpp"
#include "compx.hpp"
#include "vars.hpp"
#include "cons.hpp"
#include "detail.hpp"
#include "except.hpp"

#include <iostream>

using namespace humble;
using namespace std;

namespace {

template <typename T>
void malt_or_fail(Lex & x, string s)
{
    if (not holds_alternative<T>(x))
        throw SrcError(s);
}

Lex m_lambda(LexForm & s)
{
    if (s.v.size() < 3) throw SrcError("lambda argc");
    s.v[0] = LexOp{ OP_LAMBDA };
    if (not holds_alternative<LexForm>(s.v[1])) {
        malt_or_fail<LexNam>(s.v[1], "lambda.1 expected name");
        s.v[0] = LexOp{ OP_LAMBDA_DOT };
        s.v[1] = LexForm{{ s.v[1] }};
    } else if (auto & f = get<LexForm>(s.v[1]); is_dotform(f)) {
        s.v[0] = LexOp{ OP_LAMBDA_DOT };
        s.v[1] = without_dot(f);
    }
    LexArgs a;
    for (auto & x : get<LexForm>(s.v[1]).v) {
        malt_or_fail<LexNam>(x, "lambda params must be names");
        a.push_back(get<LexNam>(x).h);
    }
    s.v[1] = a;
    ::set<int> k{a.begin(), a.end()};
    auto u = unbound({s.v.begin() + 2, s.v.end()}, k, true);
    LexArgs b;
    for (auto i : u) b.push_back(i);
    s.v.push_back(b);
    rotate(s.v.begin() + 2, s.v.begin() + 3, s.v.end());
    return move(s);
}

Lex m_letrec(LexForm & s)
{
    if (s.v.size() < 3) throw SrcError("letrec argc");
    throw CoreError("nyi");
}

Lex m_ref(LexForm & s)
{
    if (s.v.size() < 3) throw SrcError("ref argc");
    s.v[0] = LexOp{ OP_BIND };
    if (holds_alternative<LexForm>(s.v[1])) {
        auto & f = get<LexForm>(s.v[1]);
        auto n = f.v[0];
        LexForm a{{LexOp{}, LexForm{{f.v.begin() + 1, f.v.end()}}}};
        move(s.v.begin() + 2, s.v.end(), back_inserter(a.v));
        s.v[2] = m_lambda(a);
        s.v[1] = n;
        s.v.resize(3);
        s = move(get<LexForm>(m_ref(s)));
    }
    malt_or_fail<LexNam>(s.v[1], "ref.1 expects name");
    auto & i = get<LexNam>(s.v[1]);
    ::set<int> k;
    auto u = unbound(span1(s.v, 2), k, false);
    if (u.contains(i.h)) {
        // permit define of recursive lambda by wrapping with
        // a letrec.  this leaves immediate i-e (ref i (+ i))
        // "non-working", then instead use define
        auto y = i;
        LexForm a{{LexOp{}, LexForm{{ LexForm{{ y, move(s.v[2]) }}}}, y}};
        return move(LexForm{{ LexOp{ OP_BIND }, i, m_letrec(a)}});
    }
    return move(s);
}

struct Define : Macro {
    Lex operator()(LexForm & s) override
    {
        if (s.v.size() < 3) throw SrcError("define argc");
        if (holds_alternative<LexForm>(s.v[1]))
            return m_ref(s);
        s.v[0] = LexOp{ OP_BIND };
        malt_or_fail<LexNam>(s.v[1], "define.1 expects name");
        auto d = move(s.v[2]);
        s.v[2] = LexForm{{nam_dup, d}};
        return s;
    }
};

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

struct Lambda : Macro { Lex operator()(LexForm & s) override {
    return m_lambda(s);
} };
struct Letrec : Macro { Lex operator()(LexForm & s) override {
    return m_letrec(s);
} };
struct Ref : Macro { Lex operator()(LexForm & s) override {
    return m_ref(s);
} };

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

    with_name(names, m, "lambda", make_unique<Lambda>());
    with_name(names, m, "letrec", make_unique<Letrec>());
    with_name(names, m, "ref", make_unique<Ref>());
    with_name(names, m, "define", make_unique<Define>());
    with_name(names, m, "if", make_unique<If>());

    return m;
}

static EnvEntry to_list_var(const ConsPtr & c)
{
    if (not c) {
        return make_shared<Var>(VarList{});
    } else {
        auto b = c->to_list_var();
        if (holds_alternative<VarList>(b))
            return make_shared<Var>(get<VarList>(b));
        else
            return make_shared<Var>(get<VarNonlist>(b));
    }
}

Lex to_lex(EnvEntry a)
{
    if (holds_alternative<VarCons>(*a)) {
        a = to_list_var(get<VarCons>(*a).c);
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
            // plan: to support VarRec and VarDict, will convert
            // to a LexVar with the generating expression, so that
            // lex outputs as reference vrepr; "#:(...)" that may
            // then be parsed (omitting macro-expand) as part of
            // possible (read) implementation with from_lex
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

