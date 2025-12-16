#include "xeval.hpp"
#include "debug.hpp"
#include "api.hpp"

using namespace std;

namespace humble {

vector<EnvEntry> run_each(vector<Lex> v, Env & env)
{
    vector<EnvEntry> r;
    for (auto & x : v) {
        auto y = run(x, env);
        if (holds_alternative<VarSplice>(*y)) {
            auto & u = get<VarSplice>(*y).v;
            copy(u.begin(), u.end(), back_inserter(r));
        } else {
            r.push_back(move(y));
        }
    }
    return r;
}

EnvEntry xapply(vector<EnvEntry> v)
{
    (void)v;
    // TODO: invoke fun on args
    return make_shared<Var>(VarVoid{});
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
                // TODO: if zero size, VarCons
                return make_shared<Var>(VarList{move(v)});
            }
            if constexpr (is_same_v<T, LexNonlist>) {
                auto v = run_each(z.v, env);
                if (holds_alternative<VarList>(*v.back())) {
                    auto w = move(get<VarList>(*v.back()));
                    v.pop_back();
                    move(w.v.begin(), w.v.end(), back_inserter(v));
                }
                // TODO: if last is VarCons, thus merge
                return make_shared<Var>(VarNonlist{move(v)});
            }
            if constexpr (is_same_v<T, LexNam>) {
                return env.get(z.h);
            }
            if constexpr (is_same_v<T, LexNum>) {
                return make_shared<Var>(VarNum{z.i});
            }
            if constexpr (is_same_v<T, LexBool>) {
                return make_shared<Var>(VarBool{z.b});
            }
            if constexpr (is_same_v<T, LexString>) {
                return make_shared<Var>(VarString{z.s});
            }
            if constexpr (is_same_v<T, LexSym>) {
                return make_shared<Var>(VarNam{z.h});
            }
            if constexpr (is_same_v<T, LexVoid>) {
                return make_shared<Var>(VarVoid{});
            }
            if constexpr (is_same_v<T, LexUnquote>) {
                return make_shared<Var>(VarUnquote{&z});
            }
            if constexpr (is_same_v<T, LexQuote>
                    or is_same_v<T, LexQuasiquote>) {
                CoreError("eval quote");
            }
            if constexpr (is_same_v<T, LexDot>) {
                RunError("eval dot");
            }
            if constexpr (is_same_v<T, LexForm>) {
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
    (void)f;
    (void)env;
    // TODO: perform op given args
    return make_shared<Var>(VarVoid{});
}

EnvEntry run(Lex & x, Env & env)
{
    return xeval(x, env);
    // TODO: if VarApply then tco
}

} // ns

