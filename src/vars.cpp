#include "vars.hpp"
#include "except.hpp"

using namespace std;

namespace humble {

GlobalEnv & GlobalEnv::initial()
{
    static GlobalEnv r(create_t{});
    return r;
}

GlobalEnv::GlobalEnv(create_t) : inited{false} { };

EnvEntry GlobalEnv::get(int i)
{
    if (auto it = m.find(i); it != m.end())
        return it->second;
    return {};
    // ^ TODO: instead throw LookupError having i,
    // so that in top one may inform on name or
    // re-throw as CoreError using name[i]
    // in run, as that will adapt both for expand_macros
    // and in i-e run_top.  consolidate thus in the
    // python implementation as well.
}

void GlobalEnv::set(int i, EnvEntry e) { m[i] = e; }

GlobalEnv GlobalEnv::init()
{
    GlobalEnv r(create_t{});
    if (this != &initial()) throw CoreError("init on user-env");
    if (inited) throw CoreError("init more than once");
    inited = true;
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

VarExt::VarExt(int t)
    : t(t)
    , u()
    , f()
{ }

VarExt::VarExt(const VarExt & other)
{
    if (other.f) throw RunError("non-copyable ext");
    // improve: ExtError with h, so that in run() may
    // lookup name and re-throw just like LookupError
    t = other.t;
    u = other.u;
    f = nullptr;
}

VarExt::VarExt(VarExt && other)
{
    *this = move(other);
}

VarExt & VarExt::operator=(VarExt && other)
{
    t = other.t;
    u = other.u;
    f = other.f;
    other.t = 0;
    other.u = nullptr;
    other.f = nullptr;
    return *this;
}

VarExt::~VarExt()
{
    if (f) f(u);
}

static const char * var_type_name_a[] {
    "void",        // note:  ordered as
    "number",      // Var variant index
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
    "ext",
};

const char * var_type_name(const Var & v)
{
    return var_type_name_a[v.index()];
}

} // ns

