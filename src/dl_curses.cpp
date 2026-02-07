#include "dl.hpp"
#include "fun_impl.hpp"
#include "utf.hpp"
#include <iostream>

#include <curses.h>
#include <locale.h>

using namespace humble;
using namespace std;

int t_nc_stdscr;

void delete_nc_stdscr(void *)
{
    (void)endwin();
}

EnvEntry f_nc_initscr(span<EnvEntry> args)
{
    setlocale(LC_ALL, "");  // needed for unicode wchar
    int tenths{};
    if (args.size() != 0) {
        valt_or_fail<VarNum>(args, 0, "nc-initscr");
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
    auto r = VarExt{t_nc_stdscr};
    r.u = w;
    r.f = delete_nc_stdscr;
    return make_shared<Var>(VarExt{move(r)});
}

EnvEntry f_nc_getmaxyx(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("nc-getmaxyx argc");
    auto & e = vext_or_fail({t_nc_stdscr}, args, 0, "nc-getmaxyx");
    auto w = static_cast<WINDOW *>(e.u);
    int y{};
    int x{};
    getmaxyx(w, y, x);
    return make_shared<Var>(VarList{{
        make_shared<Var>(VarNum{y}),
        make_shared<Var>(VarNum{x})}});
}

EnvEntry f_nc_addstr(span<EnvEntry> args)
{
    if (args.size() < 4) throw RunError("nc-addstr argc");
    auto & e = vext_or_fail({t_nc_stdscr}, args, 0, "nc-addstr");
    auto w = static_cast<WINDOW *>(e.u);
    valt_or_fail<VarNum>(args, 1, "nc-addstr");
    valt_or_fail<VarNum>(args, 2, "nc-addstr");
    valt_or_fail<VarString>(args, 3, "nc-addstr");
    int c = 7;
    if (args.size() >= 5) {
        valt_or_fail<VarNum>(args, 4, "nc-addstr");
        c = get<VarNum>(*args[4]).i;
    }
    auto y =  get<VarNum>(*args[1]).i;
    auto x =  get<VarNum>(*args[2]).i;
    auto & s = get<VarString>(*args[3]).s;
    wattrset(w, COLOR_PAIR(c));
    wstring t;
    size_t i{};
    while (i != s.size()) {
        auto g = utf_ref({s.begin() + i, s.end()}, 0);
        if (g.u.empty()) break;
        t.push_back(static_cast<wchar_t>(utf_value(g)));
        i += g.u.size();
    }
    mvwaddnwstr(w, y, x, t.data(), t.size());
    return make_shared<Var>(VarVoid{});
}

EnvEntry f_nc_getch(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("nc-getch argc");
    auto & e = vext_or_fail({t_nc_stdscr}, args, 0, "nc-getch");
    auto w = static_cast<WINDOW *>(e.u);
    int r = wgetch(w);
    if (r == KEY_RESIZE) {
        (void)endwin();
        exit(1);
    }
    return make_shared<Var>(VarNum{r});
}

EnvEntry f_nc_endwin(span<EnvEntry> args)
{
    (void)args;
    (void)endwin();
    return make_shared<Var>(VarVoid{});
}

extern "C" void dl_curses(void * a)
{
    auto p = static_cast<dl_arg *>(a);
    Names & n = *p->names;
    GlobalEnv & g = *p->env;
    t_nc_stdscr = n.intern("nc-stdscr");
    typedef EnvEntry (*hp)(span<EnvEntry> args);
    for (auto & p : initializer_list<pair<string, hp>>{
            { "nc-initscr", f_nc_initscr },
            { "nc-getmaxyx", f_nc_getmaxyx },
            { "nc-addstr", f_nc_addstr },
            { "nc-getch", f_nc_getch },
            { "nc-endwin", f_nc_endwin },
    }) g.set(n.intern(p.first), make_shared<Var>(VarFunHost{ p.second }));
}

