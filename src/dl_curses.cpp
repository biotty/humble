#include "dl.hpp"
#include "vars.hpp"
#include "except.hpp"
#include <iostream>

#include <curses.h>

using namespace humble;
using namespace std;

int t_stdscr;

void delete_stdscr(void *)
{
    (void)endwin();
}

EnvEntry f_initscr(span<EnvEntry> args)
{
    int tenths{};
    if (args.size() != 0) {
        if (not holds_alternative<VarNum>(*args[0]))
            throw RunError("initscr args[0] takes number");
        tenths = get<VarNum>(*args[0]).i;
    }
    auto w = initscr();
    noecho();
    curs_set(0);
    start_color();
    if (tenths) halfdelay(tenths);
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
    init_pair(4, COLOR_CYAN, COLOR_BLACK);
    init_pair(5, COLOR_BLUE, COLOR_BLACK);
    init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(7, COLOR_WHITE, COLOR_BLACK);
    auto r = VarExt{t_stdscr};
    r.u = w;
    r.f = delete_stdscr;
    return make_shared<Var>(VarExt{move(r)});
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

