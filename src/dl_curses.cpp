#include "dl.hpp"
#include "vars.hpp"
#include <iostream>

using namespace humble;
using namespace std;

int t_stdscr;

EnvEntry f_initscr(span<EnvEntry> args)
{
    (void)args;
    cout << "TODO" << endl;
    return make_shared<Var>(VarVoid{});
}

extern "C" void dl_curses(void * a)
{
    auto p = static_cast<dl_arg *>(a);
    Names & n = *p->names;
    GlobalEnv & g = *p->env;
    t_stdscr = n.intern("stdscr");
    typedef EnvEntry (*hp)(span<EnvEntry> args);
    for (auto & p : initializer_list<pair<string, hp>>{
            { "initscr", f_initscr },
    }) g.set(n.intern(p.first), make_shared<Var>(VarFunHost{ p.second }));
}

