#include "vars.hpp"

using namespace std;

namespace humble {

Env & GlobalEnv::instance()
{
    static GlobalEnv r(create_t{});
    return r;
}

GlobalEnv::GlobalEnv(create_t) {};

EnvEntry GlobalEnv::get(int i)
{
    if (auto it = m.find(i); it != m.end())
        return it->second;
    return {};
}

void GlobalEnv::set(int i, EnvEntry e) { m[i] = e; }

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

EnvEntry FunEnv::get(int i) { return v[i]; }

void FunEnv::set(int i, EnvEntry e) { v[i] = e; }

} // ns

