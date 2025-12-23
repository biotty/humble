#include "compx.hpp"
#include "debug.hpp"
#include "detail.hpp"
#include "except.hpp"
#include <sstream>

using namespace humble;
using namespace std;

namespace {

std::vector<LexEnv *> local_envs;

span<Lex> span1(span<Lex> x, size_t i)
{
    return {x.begin() + i, x.begin() + i + 1};
}

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
                op.code == OP_DEFINE) {
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
                op.code == OP_DEFINE) {
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
        } else if (op.code == OP_IMPORT or op.code == OP_IMPORT) {
            // pass
        } else {
            throw CoreError("unknown form");
        }
    }
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
            }
            if (holds_alternative<LexList>(x)) {
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
                op.code == OP_DEFINE) {
            if (not is_block)
                throw SrcError("define in non-block");
            auto i = get<LexNam>(f.v.at(1)).h;
            auto u = unbound(get<LexForm>(f.v.at(2)).v, defs, false);
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

LexForm compx(const string & s, Names & names, Macros & macros, set<int> env_keys)
{
    linenumber = 1;
    auto t = parse(s, names, macros);
    auto u = unbound(t.v, env_keys, true);
    if (u.empty()) {
        zloc_scopes(t.v, nullptr);
        return t;
    }
    ostringstream a;
    for (int y : u) {
        auto x = find_unbound(t.v, y);
        if (holds_alternative<LexVoid>(x)) {
            a << "(reportedly) " << names.get(y);
        } else {
            a << info_unbound(x, names) << "\n";
        }
    }
    throw SrcError("unbound,\n" + a.str());
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

} // ns

