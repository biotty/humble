#include "macros.hpp"
#include "compx.hpp"
#include "vars.hpp"
#include "cons.hpp"
#include "detail.hpp"
#include "except.hpp"
#include "debug.hpp"

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

// for unbound set, as vector is used
// keeps sorted as set is (invariant)
// nota bene:  not use on lambda params
//             as they are ordered.
void make_set(LexArgs & u)
{
    sort(u.begin(), u.end());
    u.erase(unique(u.begin(), u.end()),
            u.end());
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
    s.v.push_back(LexArgs{u.begin(), u.end()});
    rotate(s.v.begin() + 2, s.v.begin() + 3, s.v.end());
    return move(s);
}

void recset(LexForm & let)
{
    malt_or_fail<LexForm>(let.v[0], "recset");
    auto & f = get<LexForm>(let.v[0]);
    if (f.v.size() != 3) SrcError("recset length");
    LexArgs a = get<LexArgs>(f.v[1]);
    LexArgs b;
    for (int z : a) {
        if (z >= 0) throw CoreError("recset id");
        int n = -z;
        b.push_back(n);
        f.v.push_back(LexForm{{nam_setjj, LexNam{n, 0}, LexNam{z, 0}}});
    }
    rotate(f.v.begin() + 3, f.v.begin() + 3 + b.size(), f.v.end());
    // ^ as-if we did insert at 3 in the loop, except reversed (ok)
    LexArgs & u = get<LexArgs>(f.v[2]);
    // nota bene:  reference not taken prior to growing v
    u.push_back(NAM_SETJJ);
    for (int m : b)
        u.push_back(m);
    ::set<int> k{a.begin(), a.end()};
    for (int m : unbound({let.v.begin() + 1, let.v.end()}, k, false))
        u.push_back(m);
    make_set(u);
}

LexForm rectmp(LexForm & let, LexArgs & a)
{
    LexForm & f = get<LexForm>(let.v[0]);
    LexArgs u = get<LexArgs>(f.v[2]);
    vector<Lex> v;
    for (int z : a) {
        if (z < 0) throw CoreError("rectmp id");
        v.push_back(LexVoid{});
    }
    LexArgs w = u;
    for (int m : a) erase(w, m);
    LexForm r{{LexForm{{LexOp{ OP_LAMBDA }, a, w, move(let)}}}};
    move(v.begin(), v.end(), back_inserter(r.v));
    return r;
}

struct unzip_r {
    LexArgs a;
    vector<Lex> v;
};

unzip_r bnd_unzip(LexForm & s)
{
    unzip_r r;
    for (auto & z : s.v) {
        malt_or_fail<LexForm>(z, "let bind-item not list");
        auto & f = get<LexForm>(z);
        if (f.v.size() != 2) SrcError("let expected binding");
        auto & x = f.v[0];
        auto & y = f.v[1];
        malt_or_fail<LexNam>(x, "bind not to name");
        r.a.push_back(get<LexNam>(x).h);
        r.v.push_back(y);
    }
    return r;
}

Lex named_let(LexForm & s);

Lex m_let(LexForm & s, bool rec = false)
{
    if (s.v.size() < 2) throw SrcError("let argc");
    if (holds_alternative<LexNam>(s.v[1]))
        return named_let(s);
    LexForm lbd{{LexOp{ OP_LAMBDA }}};
    auto [a, v] = bnd_unzip(get<LexForm>(s.v[1]));
    LexArgs t;
    if (rec) {
        t = a;
        for (int & z : a) z = -z;
    }
    lbd.v.push_back(a);
    ::set<int> k{a.begin(), a.end()};
    auto u = unbound({s.v.begin() + 2, s.v.end()}, k, true);
    lbd.v.push_back(LexArgs{u.begin(), u.end()});
    if (s.v.size() == 2) lbd.v.push_back(LexVoid{});
    else move(s.v.begin() + 2, s.v.end(), back_inserter(lbd.v));
    LexForm r{{lbd}};
    move(v.begin(), v.end(), back_inserter(r.v));
    if (rec) {
        recset(r);
        r = rectmp(r, t);
    }
    return r;
}

Lex m_letx(LexForm & s, bool rec = false)
{
    if (s.v.size() < 2) throw SrcError("let* argc");
    malt_or_fail<LexForm>(s.v[1], "let*.1 expected sub-form");
    auto & f = get<LexForm>(s.v[1]);
    if (f.v.empty())
        return m_let(s);
    LexForm block{{s.v.begin() + 2, s.v.end()}};
    if (block.v.empty()) block.v.push_back(LexVoid{});
    LexArgs t;
    for (auto rit = f.v.rbegin(); rit != f.v.rend(); ++rit) {
        malt_or_fail<LexForm>(*rit, "let* bind-item not list");
        auto & z = get<LexForm>(*rit);
        if (z.v.size() != 2) SrcError("let* expected binding");
        auto & x = z.v[0];  // for let we unzip once, in func -
        auto & y = z.v[1];  // but here we wrap lambda for each
        malt_or_fail<LexNam>(x, "let* not to name");
        LexArgs a{get<LexNam>(x).h};
        if (rec) {
            int & z = a[0];
            t.push_back(z);
            z = -z;
        }
        LexForm lbd{{LexOp{ OP_LAMBDA }, a}};
        ::set<int> k{a[0]};
        LexArgs u;
        for (auto m : unbound(block.v, k, true))
            u.push_back(m);
        lbd.v.push_back(u);
        move(block.v.begin(), block.v.end(), back_inserter(lbd.v));
        LexForm r{{move(lbd), move(y)}};
        if (rec) recset(r);
        block.v = {r};
    }
    auto r = move(get<LexForm>(block.v[0]));
    if (rec) r = rectmp(r, t);
    return r;
}

Lex named_let(LexForm & s)
{
    malt_or_fail<LexNam>(s.v[1], "let not name");
    auto name = get<LexNam>(s.v[1]);
    auto [a, v] = bnd_unzip(get<LexForm>(s.v[2]));
    span<Lex> blk{s.v.begin() + 3, s.v.end()};
    ::set<int> k{a.begin(), a.end()};
    LexArgs u;
    for (int m : unbound(blk, k, true))
        u.push_back(m);
    LexForm lbd{{LexOp{ OP_LAMBDA}, a, u}};
    move(blk.begin(), blk.end(), back_inserter(lbd.v));
    LexForm bnd{{LexForm{{ name, lbd }}}};
    LexForm x{{ name }};
    move(v.begin(), v.end(), back_inserter(x.v));
    LexForm r{{ LexOp{}, bnd, x }};
    return m_let(r, true);
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
        return move(LexForm{{ LexOp{ OP_BIND }, i, m_let(a, true)}});
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
            throw SrcError("if-expr length");
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
struct Let : Macro { Lex operator()(LexForm & s) override {
    return m_let(s);
} };
struct Letx : Macro { Lex operator()(LexForm & s) override {
    return m_letx(s);
} };
struct Letrec : Macro { Lex operator()(LexForm & s) override {
    return m_let(s, true);
} };
struct Letrecx : Macro { Lex operator()(LexForm & s) override {
    return m_letx(s, true);
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
    with_name(names, m, "let", make_unique<Let>());
    with_name(names, m, "let*", make_unique<Letx>());
    with_name(names, m, "letrec", make_unique<Letrec>());
    with_name(names, m, "letrec*", make_unique<Letrecx>());
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

