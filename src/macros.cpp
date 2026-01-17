#include "macros.hpp"
#include "compx.hpp"
#include "xeval.hpp"
#include "vars.hpp"
#include "cons.hpp"
#include "except.hpp"
#include "debug.hpp"

#include <sstream>
#include <iostream>

using namespace humble;
using namespace std;

namespace {

template <typename T, typename... Ts>
bool malt_in(Lex & x)
{
    if (holds_alternative<T>(x)) return true;
    if constexpr (sizeof...(Ts) != 0) return malt_in<Ts...>(x);
    return false;
}

template <typename... Ts>
void malt_or_fail(Lex & x, string s)
{
    if (malt_in<Ts...>(x)) return;
    throw SrcError(s);
}

// used as blex - bare name equal (ign linenumber)
bool nameq(Lex & x, int h)
{
    return (holds_alternative<LexNam>(x)
            and get<LexNam>(x).h == h);
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

//
// name scope and let binding
//

Lex m_lambda(LexForm && s)
{
    if (s.v.size() < 3) throw SrcError("lambda argc");
    s.v[0] = LexOp{OP_LAMBDA};
    if (not holds_alternative<LexForm>(s.v[1])) {
        malt_or_fail<LexNam>(s.v[1], "lambda.1 expected name");
        s.v[0] = LexOp{OP_LAMBDA_DOT};
        s.v[1] = LexForm{{s.v[1]}};
    } else if (auto & f = get<LexForm>(s.v[1]); is_dotform(f)) {
        s.v[0] = LexOp{OP_LAMBDA_DOT};
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
    s.v.insert(s.v.begin() + 2, LexArgs{u.begin(), u.end()});
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
        f.v.insert(f.v.begin() + 3,
                LexForm{{nam_setjj, LexNam{n, 0}, LexNam{z, 0}}});
    }
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
    LexForm r{{LexForm{{LexOp{OP_LAMBDA}, a, w, move(let)}}}};
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

Lex named_let(LexForm && s);

Lex m_let(LexForm && s, bool rec = false)
{
    if (s.v.size() < 2) throw SrcError("let argc");
    if (holds_alternative<LexNam>(s.v[1]))
        return named_let(move(s));
    LexForm lbd{{LexOp{OP_LAMBDA}}};
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

Lex m_letx(LexForm && s, bool rec = false)
{
    if (s.v.size() < 2) throw SrcError("let* argc");
    malt_or_fail<LexForm>(s.v[1], "let*.1 expected sub-form");
    auto & f = get<LexForm>(s.v[1]);
    if (f.v.empty())
        return m_let(move(s));
    LexForm block;
    move(s.v.begin() + 2, s.v.end(), back_inserter(block.v));
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
        LexForm lbd{{LexOp{OP_LAMBDA}, a}};
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

Lex named_let(LexForm && s)
{
    malt_or_fail<LexNam>(s.v[1], "let not name");
    auto name = get<LexNam>(s.v[1]);
    auto [a, v] = bnd_unzip(get<LexForm>(s.v[2]));
    span<Lex> blk{s.v.begin() + 3, s.v.end()};
    ::set<int> k{a.begin(), a.end()};
    LexArgs u;
    for (int m : unbound(blk, k, true))
        u.push_back(m);
    LexForm lbd{{LexOp{OP_LAMBDA}, a, u}};
    move(blk.begin(), blk.end(), back_inserter(lbd.v));
    LexForm bnd{{LexForm{{name, lbd}}}};
    LexForm x{{name}};
    move(v.begin(), v.end(), back_inserter(x.v));
    LexForm r{{LexOp{}, bnd, x}};
    return m_let(move(r), true);
}

Lex m_ref(LexForm && s)
{
    if (s.v.size() < 3) throw SrcError("ref argc");
    s.v[0] = LexOp{OP_BIND};
    if (holds_alternative<LexForm>(s.v[1])) {
        auto & f = get<LexForm>(s.v[1]);
        auto n = f.v[0];
        LexForm b;
        move(f.v.begin() + 1, f.v.end(), back_inserter(b.v));
        LexForm a{{LexOp{}, move(b)}};
        move(s.v.begin() + 2, s.v.end(), back_inserter(a.v));
        s.v[2] = m_lambda(move(a));
        s.v[1] = n;
        s.v.resize(3);
        s = move(get<LexForm>(m_ref(move(s))));
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
        LexForm a{{LexOp{}, LexForm{{LexForm{{y, move(s.v[2]) }}}}, y}};
        return move(LexForm{{LexOp{OP_BIND}, i, m_let(move(a), true)}});
    }
    return move(s);
}

struct Define : MacroClone<Define> {
    Lex operator()(LexForm && s) override
    {
        if (s.v.size() < 3) throw SrcError("define argc");
        if (holds_alternative<LexForm>(s.v[1]))
            return m_ref(move(s));
        s.v[0] = LexOp{OP_BIND};
        malt_or_fail<LexNam>(s.v[1], "define.1 expects name");
        auto d = move(s.v[2]);
        s.v[2] = LexForm{{nam_dup, d}};
        return s;
    }
};

Lex m_begin(LexForm && s)
{
    LexForm a{{LexOp{}, LexForm{}}};
    move(s.v.begin() + 1, s.v.end(), back_inserter(a.v));
    return m_letx(move(a));
}

Lex m_if(LexForm && s);

struct Do : MacroClone<Do> {
    Lex operator()(LexForm && s) override
    {
        if (s.v.size() < 3) throw SrcError("do argc");
        malt_or_fail<LexForm>(s.v[1], "do parameters");
        vector<Lex> step;
        for (auto & x : get<LexForm>(s.v[1]).v) {
            malt_or_fail<LexForm>(x, "do param not form");
            auto & f = get<LexForm>(x);
            Lex y;
            if (f.v.size() == 3) {
                y = move(f.v[2]);
                f.v.resize(2); // pop
            } else if (f.v.size() == 2) {
                y = f.v[0]; // copy
            } else throw SrcError("do param length");
            step.push_back(move(y));
        }
        LexForm b{{nam_else}};
        move(step.begin(), step.end(), back_inserter(b.v));
        s.v.push_back(move(b));  // growth invalidates any refs
        malt_or_fail<LexForm>(s.v[2], "do then");
        auto & ifthen = get<LexForm>(s.v[2]);
        LexForm then_b{{LexOp{}}};
        move(ifthen.v.begin() + 1, ifthen.v.end(), back_inserter(then_b.v));
        LexForm else_b{{LexOp{}}};
        move(s.v.begin() + 3, s.v.end(), back_inserter(else_b.v));
        return named_let(LexForm{{LexOp{}, nam_else, move(s.v[1]),
                m_if(LexForm{{LexOp{}, move(ifthen.v[0]),
                        m_begin(move(then_b)),
                        m_begin(move(else_b))}})}});
    }
};

//
// macro-macro
//

struct UserMacro : MacroNotClone<UserMacro>
{
    string mname;
    LexArgs parms;
    bool isdot;
    LexForm block;
    Names * names;
    UserMacro(string mname, LexArgs parms, bool isdot, LexForm block, Names & names)
        : mname(mname), parms(parms), isdot(isdot), block(block), names(&names)
    {
        is_user = true;
    }

    Lex operator()(LexForm && s) override
    {
        vector<EnvEntry> args;
#ifdef DEBUG
        cout << "user-macro args: " << s << endl;
#endif
        for (auto & x : s.v)
            if (&x != &s.v[0])
                args.push_back(from_lex(x));
        auto env = OverlayEnv(GlobalEnv::instance());
        if (isdot) {
            size_t last = parms.size() - 1;
            if (args.size() < last) throw SrcError("user-macro dot argc");
            for (size_t i = 0; i != last; ++i)
                env.set(parms[i], args[i]);
            env.set(parms[last], make_shared<Var>(
                        VarList{{args.begin() + last, args.end()}}));
        } else {
            if (args.size() != parms.size())
                throw SrcError("user-macro argc");
            for (size_t i = 0; i != args.size(); ++i)
                env.set(parms[i], args[i]);
        }
        EnvEntry r;
        for (auto & x : block.v)
            r = run(x, env);
        auto x = to_lex(r);
#ifdef DEBUG
        cout << "user-macro result: " << x << endl;
#endif
        return x;
    }
};

struct MacroMacro : MacroNotClone<Macro> {
    Names * names;
    Macros * macros;
    MacroMacro(Names & names, Macros & macros)
        : names(&names)
        , macros(&macros)
    { }

    Lex operator()(LexForm && s) override
    {
        if (s.v.size() <= 3)
            throw SrcError("macro argc");
        malt_or_fail<LexNam>(s.v[1], "macro not name");
        auto & n = get<LexNam>(s.v[1]);
        bool isdot{true};
        auto & y = s.v[2];
        if (not malt_in<LexForm>(y)) {
            y = LexForm{{move(y)}};
        } else {
            isdot = is_dotform(get<LexForm>(y));
            if (isdot) y = without_dot(get<LexForm>(y));
        }
        zloc_scopes({s.v.begin() + 3, s.v.end()}, nullptr);
        LexArgs parms;
        for (auto & p : get<LexForm>(y).v) {
            malt_or_fail<LexNam>(p, "macro expected name");
            parms.push_back(get<LexNam>(p).h);
        }
        LexForm block;
        move(s.v.begin() + 3, s.v.end(), back_inserter(block.v));
        (*macros)[n.h] = make_unique<UserMacro>(names->get(n.h),
                parms, isdot, move(block), *names);
        return LexVoid{};
    }
};

struct Gensym : MacroClone<Gensym> {
    Names * names;
    Gensym(Names & names) : names(&names) { }

    Lex operator()(LexForm && s) override
    {
        if (s.v.size() != 1)
            throw SrcError("gensym argc");
        auto i = names->size();
        while (i == names->size()) {
            ostringstream oss;
            oss << "&" << i;
            names->intern(oss.str());
        }
        return LexSym{ static_cast<int>(i) };
    }
};

struct Seq : MacroClone<Seq> {
    Lex operator()(LexForm && s) override
    {
        s.v[0] = LexOp{OP_SEQ};
        return s;
    }
};

//
// conditions
//

Lex m_cond(LexForm && s)
{
    s.v[0] = LexOp{OP_COND};
    size_t n = s.v.size();
    size_t i = 1;
    for (; i != n; ++i) {
        auto & d = get<LexForm>(s.v[i]);
        if (nameq(d.v.at(0), NAM_ELSE)) {
            LexForm r;
            move(s.v.begin(), s.v.begin() + i, back_inserter(r.v));
            LexForm a{{LexOp{}}};
            move(d.v.begin() + 1, d.v.end(), back_inserter(a.v));
            LexForm f{{LexBool{true}, m_begin(move(a))}};
            r.v.push_back(move(f));
            return r;
        }
        if (nameq(d.v.at(1), NAM_THEN)) break;
        if (d.v.size() != 2) {
            vector<Lex> r{d.v[0]};
            LexForm a{{LexOp{}}};
            move(s.v.begin() + i, s.v.end(), back_inserter(a.v));
            Lex b = m_begin(move(a));
            auto & f = get<LexForm>(b);
            move(f.v.begin(), f.v.end(), back_inserter(r));
            s.v[i] = LexForm{move(r)};
        }
    }
    if (i == n) return move(s);
    auto & g = get<LexForm>(s.v[i]);
    LexForm a{{LexOp{}, LexForm{{
        nam_then, LexForm{{move(g.v.at(2)), nam_then}}}}}};
    move(s.v.begin() + i + 1, s.v.end(), back_inserter(a.v));
    LexForm t{{LexOp{}, LexForm{{
        LexForm{{nam_then, move(g.v.at(0))}}}}, m_cond(move(a))}};
    Lex x = m_let(move(t));
    LexForm m;
    move(s.v.begin(), s.v.begin() + i, back_inserter(m.v));
    m.v.push_back(LexForm{{LexBool{true}, move(x)}});
    return m;
}

Lex m_if(LexForm && s)
{
    if (s.v.size() == 3) {
        s.v.push_back(LexVoid{});
    } else if (s.v.size() != 4) {
        throw SrcError("if argc");
    }
    return LexForm{{
        LexOp{OP_COND},
            LexForm{{move(s.v[1]), move(s.v[2])}},
            LexForm{{LexBool{true}, move(s.v[3])}}
    }};
}

Lex and_r(LexForm && s)
{
    if (s.v.size() == 2) return s.v[1];
    LexForm a{{LexOp{}}};
    move(s.v.begin() + 2, s.v.end(), back_inserter(a.v));
    LexForm r{{move(s.v[1]), and_r(move(a))}};
    return LexForm{{LexOp{OP_COND}, move(r),
        LexForm{{LexBool{true}, LexBool{false}}}}};
}

Lex m_and(LexForm && s)
{
    if (s.v.size() == 1) return LexBool{true};
    return and_r(move(s));
}

Lex m_or(LexForm && s)
{
    if (s.v.size() == 1) return LexBool{false};
    LexForm a{{LexOp{}}};
    move(s.v.begin() + 2, s.v.end(), back_inserter(a.v));
    LexForm r{{LexBool{true}, m_or(move(a))}};
    LexForm m{{LexOp{}, LexForm{{LexForm{{nam_then, move(s.v[1])}}}},
        LexForm{{LexOp{OP_COND}, LexForm{{nam_then, nam_then}}, move(r)}}}};
    return m_let(move(m));
    /*
    return m_let([-99, [[nam_then, s[1]]],
        [OP_COND, [nam_then, nam_then],
            [(LEX_BOOL, True), m_or([-99, *s[2:]])]]])
*/
}

struct When : MacroClone<When> {
    Lex operator()(LexForm && s) override
    {
        if (s.v.size() < 2)
            throw SrcError("when argc");
        LexForm a{{LexOp{}}};
        move(s.v.begin() + 2, s.v.end(), back_inserter(a.v));
        return LexForm{{LexOp{OP_COND},
            LexForm{{move(s.v[1]), m_begin(move(a))}},
            LexForm{{LexBool{true}, LexVoid{}}}}};
    }
};

struct Unless : MacroClone<Unless> {
    Lex operator()(LexForm && s) override
    {
        if (s.v.size() < 2)
            throw SrcError("unless argc");
        LexForm a{{LexOp{}}};
        move(s.v.begin() + 2, s.v.end(), back_inserter(a.v));
        return LexForm{{LexOp{OP_COND},
            LexForm{{move(s.v[1]), LexVoid{}}},
            LexForm{{LexBool{true}, m_begin(move(a))}}}};
    }
};

struct Case : MacroClone<Case> {

private:

    static Lex xcase_test(Lex && t)
    {
        if (not holds_alternative<LexForm>(t)) {
            if (not nameq(t, NAM_ELSE))
                throw SrcError("case neither form nor else");
            return LexBool{true};
        }
        LexForm m{{LexOp{}}};
        auto & f = get<LexForm>(t);
        for (auto & v : f.v)
            m.v.push_back(LexForm{{nam_eqvp, v, nam_else}});
        return m_or(move(m));
    }

    static Lex xcase_target(LexForm && s)
    {
        if (not nameq(s.v[0], NAM_THEN)) {
            if (s.v.size() != 1)
                throw SrcError("case target length");
            return s.v[0];
        }
        if (s.v.size() != 2)
            throw SrcError("=> target length");
        return LexForm{{s.v[1], nam_else}};
    }

    static Lex xcase(LexForm && s)
    {
        if (not nameq(s.v.back(), NAM_ELSE))
            s.v.push_back(LexForm{{nam_else, nam_then, nam_error}});
        // deviation: from r7rs which states that result is unspecified
        // when no cases match and no "else".  if not using this result
        // in such a situation there would be no ill-effect.
        // rationale: hard to find issues may arise if returning the
        // VOID value, so early detection saves this problem that is
        // then addressed by programming a propper else-case.
        // alt: instead have [nam_else, (LEX_VOID,)] above.
        LexForm m{{LexOp{}}};
        for (auto & ce : s.v) {
            auto & f = get<LexForm>(ce);
            LexForm a{{LexOp{}, xcase_test(move(f.v[0]))}};
            if (not nameq(f.v[0], NAM_ELSE) or nameq(f.v[1], NAM_THEN)) {
                LexForm x;
                move(f.v.begin() + 1, f.v.end(), back_inserter(x.v));
                a.v.push_back(xcase_target(move(x)));
            } else {
                LexForm x{{LexOp{}}};
                move(f.v.begin() + 1, f.v.end(), back_inserter(x.v));
                a.v.push_back(m_begin(move(x)));
            }
            m.v.push_back(m_and(move(a)));
        }
        return m_or(move(m));
    }

public:

    Lex operator()(LexForm && s) override
    {
        if (s.v.size() < 2)
            throw SrcError("case argc");
        LexForm a;
        move(s.v.begin() + 2, s.v.end(), back_inserter(a.v));
        // note: abuse of "else" as switch variable name
        return m_letx(LexForm{{LexOp{},
                LexForm{{LexForm{{nam_else, s.v[1]}}}},
                xcase(move(a))}});
    }
};

//
// import
//

Macros i_macros;

struct Import : MacroNotClone<Import> {
    Names * names;
    Macros * macros;
    SrcOpener * opener;
    Import(Names & names, Macros & macros, SrcOpener & opener)
        : names(&names)
        , macros(&macros)
        , opener(&opener)
    { }

private:

    static int get_nam_sym(Lex & x, bool & is_sym)
    {
        malt_or_fail<LexNam, LexSym>(x, "expected name or symbol");
        is_sym = holds_alternative<LexSym>(x);
        return is_sym ? get<LexSym>(x).h : get<LexNam>(x).h;
    }

public:

    Lex operator()(LexForm && s) override
    {

        auto u_fn = opener->filename;
        if (s.v.size() != 2 and s.v.size() != 3) throw SrcError("import argc");
        malt_or_fail<LexString>(s.v[1], "import.1 expects name");
        auto e_macros = clone_macros(i_macros);
        e_macros[NAM_MACRO] = make_unique<MacroMacro>(*names, e_macros);
        e_macros[NAM_IMPORT] = make_unique<Import>(*names, e_macros, *opener);
        auto src = (*opener)(get<LexString>(s.v[1]).s);
        auto u_ln = linenumber;
        auto r = compx(src, *names, e_macros, GlobalEnv::instance().keys());
        string prefix_s;
        bool is_prefix_sym{};
        if (s.v.size() == 3) {
            int h = get_nam_sym(s.v[2], is_prefix_sym);
            prefix_s = names->get(h);
        }
        auto & f = get<LexForm>(r.v[0]);
        LexImport set_up;
        if (get<LexOp>(f.v[0]).code != OP_EXPORT)
            throw SrcError("missing export");
        for (auto & n : f.v) {
            if (&n == &f.v[0]) continue;
            bool is_sym;
            int x = get_nam_sym(n, is_sym);
            int y = x;
            if (not is_sym) {
                if (not prefix_s.empty())
                    y = names->intern(prefix_s + names->get(x));
                set_up.a.push_back(y);
                set_up.b.push_back(x);
            } else {
                if (not prefix_s.empty() and is_prefix_sym)
                    y = names->intern(prefix_s + names->get(x));
                if (not e_macros.contains(x))
                    throw SrcError("no macro to import");
                (*macros)[y] = move(e_macros[x]);
            }
        }
        opener->filename = u_fn;
        linenumber = u_ln;
        LexForm t{{LexOp{OP_IMPORT}, move(set_up)}};
        move(r.v.begin(), r.v.end(), back_inserter(t.v));
        return t;
    }
};

struct Export : MacroClone<Export> {
    Lex operator()(LexForm && s) override {
        s.v[0] = LexOp{OP_EXPORT};
        return move(s);
    }
};

//
// wrap macro-functions that were needed as such because invoked
//
struct Lambda : MacroClone<Lambda> { Lex operator()(LexForm && s) override {
    return m_lambda(move(s));
} };
struct Ref : MacroClone<Ref> { Lex operator()(LexForm && s) override {
    return m_ref(move(s));
} };
struct Let : MacroClone<Let> { Lex operator()(LexForm && s) override {
    return m_let(move(s));
} };
struct Letx : MacroClone<Letx> { Lex operator()(LexForm && s) override {
    return m_letx(move(s));
} };
struct Letrec : MacroClone<Letrec> { Lex operator()(LexForm && s) override {
    return m_let(move(s), true);
} };
struct Letrecx : MacroClone<Letrecx> { Lex operator()(LexForm && s) override {
    return m_letx(move(s), true);
} };
struct Begin : MacroClone<Begin> { Lex operator()(LexForm && s) override {
    return m_begin(move(s));
} };
struct Cond : MacroClone<Cond> { Lex operator()(LexForm && s) override {
    return m_cond(move(s));
} };
struct If : MacroClone<If> { Lex operator()(LexForm && s) override {
    return m_if(move(s));
} };
struct And : MacroClone<And> { Lex operator()(LexForm && s) override {
    return m_and(move(s));
} };
struct Or : MacroClone<Or> { Lex operator()(LexForm && s) override {
    return m_or(move(s));
} };

//
// assert as expected no name already used at this point
//
void with_name(Names & n, Macros & m, string name, unique_ptr<Macro> nm)
{
    int i = n.size();
    if (i != n.intern(name))
        throw CoreError("non-unique macro name");
    m[i] = move(nm);
}

} // ans

namespace humble {

void init_macros(Macros & m, Names & names, SrcOpener & opener)
{
    m = qt_macros();

    auto env_keys = GlobalEnv::instance().keys();
    (void)env_keys;

    m[NAM_MACRO] = make_unique<MacroMacro>(names, m);
    m[NAM_IMPORT] = make_unique<Import>(names, m, opener);
    with_name(names, m, "lambda", make_unique<Lambda>());
    with_name(names, m, "let", make_unique<Let>());
    with_name(names, m, "let*", make_unique<Letx>());
    with_name(names, m, "letrec", make_unique<Letrec>());
    with_name(names, m, "letrec*", make_unique<Letrecx>());
    with_name(names, m, "begin", make_unique<Begin>());
    with_name(names, m, "do", make_unique<Do>());
    with_name(names, m, "ref", make_unique<Ref>());
    with_name(names, m, "define", make_unique<Define>());
    with_name(names, m, "cond", make_unique<Cond>());
    with_name(names, m, "if", make_unique<If>());
    with_name(names, m, "when", make_unique<When>());
    with_name(names, m, "and", make_unique<And>());
    with_name(names, m, "or", make_unique<Or>());
    with_name(names, m, "unless", make_unique<Unless>());
    with_name(names, m, "case", make_unique<Case>());
    with_name(names, m, "export", make_unique<Export>());
    with_name(names, m, "gensym", make_unique<Gensym>(names));
    with_name(names, m, "seq", make_unique<Seq>());
}

void macros_init(Macros & macros)
{
    i_macros = clone_macros(macros);
}

} // ns

