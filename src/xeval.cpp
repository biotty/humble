#include "xeval.hpp"
#include "debug.hpp"
#include "compx.hpp"
#include "cons.hpp"
#include "except.hpp"
#include <span>

using namespace std;

namespace humble {

EnvEntry xeval(Lex & x, Env & env);

struct FunOps {
    FunEnv captured;
    LexEnv * local_env;
    bool dot;
    span<Lex> block;
};

EnvEntry tco(FunOps * f, span<EnvEntry> args)
{
    // cout << "tco\n";
    EnvEntry v;
    // ^ alt: = make_shared<Var>(VarVoid{}); to avoid nullptr
    bool done = false;
    while (not done) {
        auto env = f->local_env
            ->activation(f->captured, f->dot, args);
        done = true;
        for (auto & w : f->block) {
            v = xeval(w, env);
            if (holds_alternative<VarApply>(*v)) {
                auto & a = get<VarApply>(*v).a;
                args = span<EnvEntry>{a.begin() + 1, a.end()};
                auto & z = get<VarFunOps>(*a.at(0));
                if (&w != &f->block.back()) {
#ifdef DEBUG
                    cout << "rec-apply\n";
#endif
                    v = tco(&*z.f, args);
                } else {
#ifdef DEBUG
                    cout << "iter-apply\n";
#endif
                    f = &*z.f;
                    done = false;
                }
            }
        }
    }
    return v;
}

EnvEntry fun_call(vector<EnvEntry> v)
{
#ifdef DEBUG
    cout << "fun-call\n";
#endif
    auto args = span<EnvEntry>{v.begin() + 1, v.end()};
    auto z = v.at(0);
    if (holds_alternative<VarFunHost>(*z)) {
#ifdef DEBUG
        cout << "native-fun\n";
#endif
        auto q = get<VarFunHost>(*z).p(args);
        if (not holds_alternative<VarApply>(*q))
            return q;
        auto & b = get<VarApply>(*q).a;
        args = span<EnvEntry>(b.begin() + 1, b.end());
        z = b.at(0);
    }
    return tco(&*get<VarFunOps>(*z).f, args);
}

VarFunOps make_fun(Env & up, span<Lex> x, int op_code)
{
    // cout << get<LexNum>(x[2]).i << " make_fun x\n";
    // cout << &get<LexNum>(x[2]) << " make_fun x\n";
    auto local_env = get<LexEnv *>(x[0]);
    auto & a = get<LexArgs>(x[1]);
    FunEnv captured{a.size()};
    size_t i = 0;
    for (auto k : get<LexArgs>(x[1]))
        captured.set(i++, up.get(k));
    auto fun_block = span<Lex>{x.begin() + 2, x.end()};
    bool dot = (op_code == OP_LAMBDA_DOT);
    // cout << get<LexNum>(fun_block.front()).i << " make_fun in block\n";
    // cout << &fun_block.front() << " make_fun block\n";
    // cout << fun_block.size() << " make_fun block size\n";
    return { make_shared<FunOps>(captured, local_env, dot, fun_block) };
}

vector<EnvEntry> run_each(span<Lex> v, Env & env)
{
    vector<EnvEntry> r;
    for (auto & x : v) {
        auto y = run(x, env);
        if (holds_alternative<VarSplice>(*y)) {
            auto & u = get<VarSplice>(*y).v;
            copy(u.begin(), u.end(), back_inserter(r));
        } else {
            r.push_back(y);
        }
    }
    return r;
}

EnvEntry xapply(vector<EnvEntry> v)
{
    if (holds_alternative<VarFunOps>(*v.at(0)))
        return make_shared<Var>(VarApply{v});
    if (holds_alternative<VarFunHost>(*v.at(0)))
        return get<VarFunHost>(*v.at(0)).p({v.begin() + 1, v.end()});
    throw RunError("apply non-fun");
}

EnvEntry xeval_op(LexForm & f, Env & env);

EnvEntry xeval(Lex & x, Env & env)
{
#ifdef DEBUG
    cout << "eval: " << x << "\n";
#endif
    return visit([&env](auto && z) -> EnvEntry {
            using T = decay_t<decltype(z)>;
            if constexpr (is_same_v<T, LexList>) {
                auto v = run_each(z.v, env);
                if (v.empty())
                    return make_shared<Var>(VarCons{});
                return make_shared<Var>(VarList{move(v)});
            }
            if constexpr (is_same_v<T, LexNonlist>) {
                auto v = run_each(z.v, env);
                if (v.empty()) throw CoreError("empty nonlist");
                if (holds_alternative<VarList>(*v.back())) {
                    auto w = move(get<VarList>(*v.back()));
                    v.pop_back();
                    move(w.v.begin(), w.v.end(), back_inserter(v));
                    return make_shared<Var>(VarList{move(v)});
                }
                if (holds_alternative<VarCons>(*v.back())) {
                    auto c = make_shared<Var>(Cons::from_list(
                                {v.begin(), v.begin() + v.size() - 1}));
                    Cons::last->d = v.back();
                    return c;
                }
                return make_shared<Var>(VarNonlist{move(v)});
            }
            if constexpr (is_same_v<T, LexNam>)
                return env.get(z.h);
            if constexpr (is_same_v<T, LexNum>)
                return make_shared<Var>(VarNum{z.i});
            if constexpr (is_same_v<T, LexBool>)
                return make_shared<Var>(VarBool{z.b});
            if constexpr (is_same_v<T, LexString>)
                return make_shared<Var>(VarString{z.s});
            if constexpr (is_same_v<T, LexSym>)
                return make_shared<Var>(VarNam{z.h});
            if constexpr (is_same_v<T, LexVoid>)
                return make_shared<Var>(VarVoid{});
            if constexpr (is_same_v<T, LexUnquote>)
                return make_shared<Var>(VarUnquote{&z});
            if constexpr (is_same_v<T, LexQuote>
                    or is_same_v<T, LexQuasiquote>)
                CoreError("eval quote");
            if constexpr (is_same_v<T, LexDot>)
                RunError("invalid use of dot");
            if constexpr (is_same_v<T, LexForm>) {
                // cout << &z.v.back() << " xeval form back\n";
                if (z.v.empty()) throw CoreError("empty form");
                if (not holds_alternative<LexOp>(z.v[0]))
                    return xapply(run_each(z.v, env));
                return xeval_op(z, env);
            }
            throw CoreError("eval unknown");
    }, x);
}

EnvEntry xeval_op(LexForm & f, Env & env)
{
    auto & op = get<LexOp>(f.v.at(0));
    if (op.code == OP_BIND) {
        env.set(get<LexNam>(f.v.at(1)).h,
                run(f.v.at(2), env));
    } else if (op.code == OP_LAMBDA or op.code == OP_LAMBDA_DOT) {
        // cout << get<LexNum>(f.v.back()).i << " xeval_op\n";
        // cout << &f.v.back() << " xeval_op\n";
        return make_shared<Var>(make_fun(env,
                    {f.v.begin() + 1, f.v.end()}, op.code));
    } else if (op.code == OP_COND) {
        for (auto yi = f.v.begin() + 1; yi != f.v.end(); ++yi) {
            auto y = get<LexForm>(*yi);
            auto t = run(y.v.at(0), env);
            if (not holds_alternative<VarBool>(*t) or get<VarBool>(*t).b)
                return xeval(y.v.at(1), env);
        }
        throw RunError("all cond #f");
    } else if (op.code == OP_IMPORT) {
        OverlayEnv e{GlobalEnv::instance()};
        for (auto zi = f.v.begin() + 2; zi != f.v.end(); ++zi)
            run(*zi, e);
        auto & m = get<LexImport>(f.v.at(1));
        for (size_t i = 0; i != m.a.size(); ++i) {
            auto p = e.get(m.b.at(i));
            if (not p) throw RunError("no such name for export");
            env.set(m.a.at(i), p);
        }
    } else if (op.code == OP_SEQ) {
        EnvEntry r;
        for (auto yi = f.v.begin() + 1; yi != f.v.end(); ++yi)
            r = run(*yi, env);
        return r;
    } else if (op.code == OP_EXPORT) {
        // pass
    } else {
        throw CoreError("unknown op");
    }
    return make_shared<Var>(VarVoid{});
}

EnvEntry run(Lex & x, Env & env)
{
    auto y = xeval(x, env);
    if (not holds_alternative<VarApply>(*y))
        return y;
    auto & a = get<VarApply>(*y).a;
    auto args = span<EnvEntry>(a.begin() + 1, a.end());
    return tco(&*get<VarFunOps>(*a.at(0)).f, args);
}

} // ns

