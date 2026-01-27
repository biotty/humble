#include "compx.hpp"
#include "debug.hpp"
#include "except.hpp"
#include "cons.hpp"
#include "parse.hpp"
#include "xeval.hpp"
#include "debug.hpp"
#include <iostream>
#include <sstream>

using namespace humble;
using namespace std;

namespace {

std::vector<LexEnv *> local_envs;

Lex find_unbound(span<Lex> t, int y)
{
    auto b = [](Lex & q) { return not holds_alternative<LexVoid>(q); };
    for (auto & x : t) {
        if (not holds_alternative<LexForm>(x)) {
            if (holds_alternative<LexNam>(x)) {
                if (get<LexNam>(x).h == y)
                    return x;
            } else if (holds_alternative<LexList>(x)) {
                if (auto r = find_unbound(get<LexList>(x).v, y); b(r))
                    return r;
            } else if (holds_alternative<LexNonlist>(x)) {
                if (auto r = find_unbound(get<LexNonlist>(x).v, y); b(r))
                    return r;
            }
        } else if (auto & f = get<LexForm>(x);
                not holds_alternative<LexOp>(f.v.at(0))) {
            if (auto r = find_unbound(f.v, y); b(r))
                return r;
        } else if (auto & op = get<LexOp>(f.v.at(0));
                op.code == OP_BIND) {
            if (auto r = find_unbound(span1(f.v, 2), y); b(r))
                return r;
        } else if (op.code == OP_LAMBDA or op.code == OP_LAMBDA_DOT) {
            if (auto & a = get<LexArgs>(f.v.at(2));
                    find(a.begin(), a.end(), y) != a.end())
                return find_unbound({f.v.begin() + 3, f.v.end()}, y);
        } else if (op.code == OP_COND or op.code == OP_SEQ) {
            if (auto r = find_unbound({f.v.begin() + 1, f.v.end()}, y); b(r))
                return r;
        } else if (op.code == OP_IMPORT or op.code == OP_EXPORT) {
            // pass
        } else {
            throw CoreError("unknown form");
        }
    }
    return LexVoid{};  // <-- use to indicate not found
}

string info_unbound(Lex & x, Names & names)
{
    auto & n = get<LexNam>(x);
    ostringstream oss;
    oss << "line " << n.line << ": " << names.get(n.h);
    return oss.str();
}

} // and

namespace humble {

set<int> unbound(span<Lex> t, set<int> & defs, bool is_block)
{
    auto r = set<int>();
    auto from_branches = set<int>();
    for (auto & x : t) {
        if (not holds_alternative<LexForm>(x)) {
            if (holds_alternative<LexNam>(x)) {
                if (auto h = get<LexNam>(x).h;
                        not defs.contains(h))
                    r.insert(get<LexNam>(x).h);
            } else if (holds_alternative<LexList>(x)) {
                auto u = unbound(get<LexList>(x).v, defs, false);
                r.insert(u.begin(), u.end());
            } else if (holds_alternative<LexNonlist>(x)) {
                auto u = unbound(get<LexNonlist>(x).v, defs, false);
                r.insert(u.begin(), u.end());
            }
        } else if (auto & f = get<LexForm>(x);
                not holds_alternative<LexOp>(f.v.at(0))) {
            auto u = unbound(f.v, defs, false);
            r.insert(u.begin(), u.end());
        } else if (auto & op = get<LexOp>(f.v[0]);
                op.code == OP_BIND) {
            if (not is_block)
                throw SrcError("define in non-block");
            auto i = get<LexNam>(f.v.at(1)).h;
            auto u = unbound(span1(f.v, 2), defs, false);
#ifdef DEBUG
            if (defs.contains(i)) {
                cout << "re-define\n";
            }
#endif
            r.insert(u.begin(), u.end());
            defs.insert(i);
        } else if (op.code == OP_LAMBDA or op.code == OP_LAMBDA_DOT) {
            auto a = get<LexArgs>(f.v.at(2));
            from_branches.insert(a.begin(), a.end());
        } else if (op.code == OP_COND) {
            auto u = unbound({f.v.begin() + 1, f.v.end()}, defs, false);
            r.insert(u.begin(), u.end());
        } else if (op.code == OP_SEQ) {
            for (size_t y = 1; y != f.v.size(); y++) {
                auto u = unbound(span1(f.v, y), defs, true);
                r.insert(u.begin(), u.end());
            }
        } else if (op.code == OP_IMPORT) {
            auto w = get<LexImport>(f.v.at(1)).a;
            defs.insert(w.begin(), w.end());
        } else if (op.code == OP_EXPORT) {
            // pass
        } else {
            throw CoreError("unknown form");
        }
    }
    vector<int> w(from_branches.size());
    auto e = set_difference(from_branches.begin(), from_branches.end(),
            defs.begin(), defs.end(), w.begin());
    r.insert(w.begin(), e);
    return r;
}

void report_unbound(set<int> u, LexForm & t, Names & names)
{
    ostringstream a;
    for (int y : u) {
        auto x = find_unbound(t.v, y);
        if (holds_alternative<LexVoid>(x)) {
            a << "\n(reportedly) " << names.get(y);
        } else {
            a << "\n" << info_unbound(x, names);
        }
    }
    throw SrcError("unbound," + a.str());
}

void zloc_scopes(span<Lex> t, LexEnv * local_env)
{
    for (auto & x : t) {
        if (not holds_alternative<LexForm>(x)) {
            if (holds_alternative<LexNam>(x)) {
                if (local_env) {
                    auto & n = get<LexNam>(x);
                    n.h = local_env->rewrite_name(n.h);
                }
            } else if (holds_alternative<LexList>(x)) {
                zloc_scopes(get<LexList>(x).v, local_env);
            } else if (holds_alternative<LexNonlist>(x)) {
                zloc_scopes(get<LexNonlist>(x).v, local_env);
            }
        } else if (auto & f = get<LexForm>(x);
                not holds_alternative<LexOp>(f.v.at(0))) {
            zloc_scopes(f.v, local_env);
        } else if (auto & op = get<LexOp>(f.v[0]);
                op.code == OP_BIND) {
            if (local_env) {
                auto & n = get<LexNam>(f.v.at(1));
                n.h = local_env->rewrite_name(n.h);
            }
            zloc_scopes(span1(f.v, 2), local_env);
        } else if (op.code == OP_LAMBDA or op.code == OP_LAMBDA_DOT) {
            auto & c = get<LexArgs>(f.v.at(2));
            auto fun_env = new LexEnv(get<LexArgs>(f.v[1]), c);
            local_envs.push_back(fun_env);
            zloc_scopes({f.v.begin() + 3, f.v.end()}, fun_env);
            f.v[1] = fun_env;
            if (local_env)
                c = local_env->rewrite_names(c);
        } else if (op.code == OP_COND or op.code == OP_SEQ) {
            zloc_scopes({f.v.begin() + 1, f.v.end()}, local_env);
        } else if (op.code == OP_IMPORT or op.code == OP_EXPORT) {
            // pass
        } else {
            throw CoreError("unknown form");
        }
    }
}

LexForm compx(const string & s, Names & names, Macros & macros, set<int> env_keys)
{
    auto t = parse(s, names, macros);
    auto u = unbound(t.v, env_keys, true);
    if (u.empty()) {
        zloc_scopes(t.v, nullptr);
        return t;
    }
    report_unbound(u, t, names);
    terminate();
}

void compx_dispose()
{
    for (auto p : local_envs)
        delete p;
    local_envs.clear();
}

LexEnv::LexEnv(const vector<int> & parms, const vector<int> & capture)
{
        n_parms = parms.size();
        names = parms;
        names.insert(names.end(), capture.begin(), capture.end());
        n_init = names.size();
}

vector<int> LexEnv::parms() const
{
    return {names.begin(), names.begin() + n_parms};
}

vector<int> LexEnv::capture() const
{
    return {names.begin() + n_parms, names.end()};
}

vector<int> LexEnv::rewrite_names(const vector<int> & c)
{
    vector<int> r;
    for (auto & n : c)
        r.push_back(rewrite_name(n));
    return r;
}

int LexEnv::rewrite_name(int n)
{
    auto it = find(names.begin(), names.end(), n);
    if (it != names.end())
        return distance(names.begin(), it);
    int i = names.size();
    names.push_back(n);
    return i;
}

FunEnv LexEnv::activation(FunEnv & captured, bool dot, span<EnvEntry> args)
{
    FunEnv env(names.size());
    auto & c = captured.v;
    copy(c.begin(), c.end(), env.v.begin() + n_parms);
    if (dot) {
        size_t last = n_parms - 1;
        if (args.size() < last)
            throw RunError("fun-dot expected more args");
        copy(args.begin(), args.begin() + last, env.v.begin());
        env.set(last, make_shared<Var>(VarList{}));
        copy(args.begin() + last, args.end(),
                back_inserter(get<VarList>(*env.get(last)).v));
    } else {
        if (args.size() != n_parms)
            throw RunError("fun bad arg count");
        copy(args.begin(), args.end(), env.v.begin());
    }
    return env;
}

static EnvEntry to_list_var(const ConsPtr & c)
{
    if (not c) {
        return make_shared<Var>(VarList{});
    } else {
        auto b = c->to_list_var();
        if (holds_alternative<VarList>(b))
            return make_shared<Var>(get<VarList>(move(b)));
        else
            return make_shared<Var>(get<VarNonlist>(move(b)));
    }
}

Lex to_lex(EnvEntry a)
{
    // cerr << "to_lex\n";
    if (not a) throw CoreError("mute variable");
    if (holds_alternative<VarCons>(*a)) {
        // cerr << "holds cons\n";
        a = to_list_var(get<VarCons>(*a).c);
    }
    // cerr << "to_lex visit\n";
    return visit([&a](auto && q) -> Lex {
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
            } else if constexpr (is_same_v<T, VarRec>) {
                vector<Lex> v;
                for (auto & y : q.v)
                    v.push_back(quote(to_lex(y)));
                return LexRec{ v };
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
                throw CoreError("to lex not handled, "
                        + string{var_type_name(*a)});
            }
    }, *a);
}

EnvEntry from_lex(Lex & x)
{
    return visit([&x](auto & q) -> EnvEntry {
            using T = decay_t<decltype(q)>;
            if constexpr (is_same_v<T, LexForm>) {
                bool isd = is_dotform(q);
                if (isd) q = without_dot(q);
                vector<EnvEntry> v;
                for (auto & w : q.v)
                    v.push_back(from_lex(w));
                if (isd) return make_shared<Var>(VarNonlist{v});
                return make_shared<Var>(VarList{v});
            } else if constexpr (is_same_v<T, LexList>) {
                vector<Lex> v{ nam_list };
                move(q.v.begin(), q.v.end(), back_inserter(v));
                Lex r = LexForm{ v };
                return from_lex(r);
            } else if constexpr (is_same_v<T, LexNonlist>) {
                vector<Lex> v{ nam_nonlist };
                move(q.v.begin(), q.v.end(), back_inserter(v));
                Lex r = LexForm{ v };
                return from_lex(r);
            } else if constexpr (is_same_v<T, LexSym>) {
                vector<Lex> v{ nam_quote, LexNam{q.h, 0} };
                Lex r = LexForm{ v };
                return from_lex(r);
            } else if constexpr (is_same_v<T, LexBool>) {
                return make_shared<Var>(VarBool{ q.b });
            } else if constexpr (is_same_v<T, LexNum>) {
                return make_shared<Var>(VarNum{ q.i });
            } else if constexpr (is_same_v<T, LexString>) {
                return make_shared<Var>(VarString{ q.s });
            } else if constexpr (is_same_v<T, LexRec>) {
                vector<EnvEntry> v;
                for (auto & w : q.v)
                    v.push_back(run(w, nullenv));
                return make_shared<Var>(VarRec{ v });
            } else if constexpr (is_same_v<T, LexNam>) {
                return make_shared<Var>(VarNam{ q.h });
            } else if constexpr (is_same_v<T, LexVoid>) {
                return make_shared<Var>(VarVoid{});
            } else {
                ostringstream oss;
                oss << "lex#" << x.index();
                throw CoreError("from lex not handled, "
                        + oss.str());
            }
    }, x);
}

void print(EnvEntry a, Names & n, std::ostream & os)
{
    if (holds_alternative<VarCons>(*a)) {
        auto & c = get<VarCons>(*a).c;
        if (not c) {
            a = make_shared<Var>(VarList{});
        } else {
            std::visit([&a](auto && w) {
                a = make_shared<Var>(w);
            }, c->to_list_var());
        }
    }
    visit([&n, &os, &a](auto && z) {
            using T = decay_t<decltype(z)>;
            if constexpr (is_same_v<T, VarList>) {
                if (z.v.empty()) {
                    os << "'()";
                    return;
                }
                char c = '(';
                for (auto & w : z.v) {
                    os << c;
                    print(w, n, os);
                    c = ' ';
                }
                os << ')';
            } else if constexpr (is_same_v<T, VarNonlist>) {
                auto j = z.v.size();
                if (j < 2) throw CoreError("short nonlist");
                char c = '(';
                for (auto & w : z.v) {
                    if (0 == --j) os << " .";
                    os << c;
                    print(w, n, os);
                    c = ' ';
                }
                os << ')';
            } else if constexpr (is_same_v<T, VarBool>) {
                os << (z.b ? "#t" : "#f");
            } else if constexpr (is_same_v<T, VarNum>) {
                os << z.i;
            } else if constexpr (is_same_v<T, VarString>) {
                os << '"' << escape(z.s) << '"';
            } else if constexpr (is_same_v<T, VarRec>) {
                os << "#r";
                print(make_shared<Var>(VarList{ z.v }), n, os);
            } else if constexpr (is_same_v<T, VarNam>) {
                os << n.get(z.h);
            } else if constexpr (is_same_v<T, VarVoid>) {
                os << "#void";
            } else if constexpr (is_same_v<T, VarExt>) {
                os << "#~" << n.get(z.t);
            } else {
                throw CoreError("unexpected var for print, "
                        + string{var_type_name(*a)});
            }
    }, *a);
}

bool warn_off;

void warn(const std::string & m)
{
    if (warn_off) return;
    cerr << "warn: " << m << endl;
}

void warn(const string & m, span<EnvEntry> e)
{
    if (warn_off) return;
    int i{};
    Names nonames;
    cerr << "run-warn: " << m << ",";
    for (auto & a : e) {
        cerr << "\nwarn-args[" << i++ << "]: " << &*a << " ";
        print(a, nonames, cerr);
    }
    cerr << endl;
}

void warn(const string & m, span<Lex> & t)
{
    if (warn_off) return;
    int i{};
    Names nonames;
    cerr << "src-warn: " << m << ",";
    for (auto & x : t) {
        cerr << "\nwarn-args[" << i++ << "]: " << &x << " ";
        cerr << "lex#" << x.index();
    }
    cerr << endl;
}

} // ns

