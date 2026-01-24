#include "vars.hpp"
#include "except.hpp"

using namespace std;

namespace humble {

GlobalEnv & GlobalEnv::instance()
{
    static GlobalEnv r(create_t{});
    return r;
}

GlobalEnv::GlobalEnv(create_t) : initial{false} { };

EnvEntry GlobalEnv::get(int i)
{
    if (auto it = m.find(i); it != m.end())
        return it->second;
    return {};
    // ^ TODO: instead throw LookupError having i,
    // so that in top one may inform on name or
    // re-throw as CoreError using name[i]
}

void GlobalEnv::set(int i, EnvEntry e) { m[i] = e; }

GlobalEnv GlobalEnv::init()
{
    GlobalEnv r(create_t{});
    if (this != &instance()) throw CoreError("init on user-env");
    if (initial) throw CoreError("init more than once");
    initial = true;
    r.m = m;
    return r;
}

::set<int> GlobalEnv::keys()
{
    ::set<int> r;
    for (const auto & p : m)
        r.insert(p.first);
    return r;
}

OverlayEnv::OverlayEnv(Env & d) : m(GlobalEnv::create_t{}), e(d) {}

EnvEntry OverlayEnv::get(int i)
{
    if (auto p = m.get(i); p)
        return p;
    return e.get(i);
}

void OverlayEnv::set(int i, EnvEntry e) { m.set(i, e); }

FunEnv::FunEnv(size_t n) : v(n) {}

FunEnv::FunEnv(std::initializer_list<EnvEntry> w) : v(w) {}

EnvEntry FunEnv::get(int i)
{
    return v[i];
}

void FunEnv::set(int i, EnvEntry e) { v[i] = e; }

EnvEntry NullEnv::get(int)
{
    throw CoreError("nullenv lookup");
}

void NullEnv::set(int, EnvEntry) { };

static const char * var_type_name_a[] {
    "void",                     // note:  ordered as
    "number",                   // Var variant index
    "bool",
    "name",
    "string",
    "list",
    "nonlist",
    "splice",
    "unquote",
    "fun-ops",
    "fun-host",
    "lz-apply",
    "cons",
    "rec",
};

const char * var_type_name(const Var & v)
{
    return var_type_name_a[v.index()];
}

} // ns

