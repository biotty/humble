#include "functions.hpp"
#include "except.hpp"
#include "detail.hpp"
#include "vars.hpp"
#include "cons.hpp"
#include "debug.hpp"

using namespace humble;
using namespace std;

namespace {

template <typename T>
void valt_or_fail(Var & v, string s)
{
    if (not holds_alternative<T>(v))
        throw RunError(s);
}

EnvEntry f_list(span<EnvEntry> args)
{
    return make_shared<Var>(VarList{ { args.begin(), args.end() } });
}

EnvEntry f_nonlist(span<EnvEntry> args)
{
    return make_shared<Var>(VarNonlist{ { args.begin(), args.end() } });
}

EnvEntry f_pluss(span<EnvEntry> args)
{
    long long r{};
    for (auto & a : args) {
        valt_or_fail<VarNum>(*a, "+ arg expected number");
        r += get<VarNum>(*a).i;
    }
    return make_shared<Var>(VarNum{ r });
}

EnvEntry setjj(span<EnvEntry> args)
{
    if (&*args[0] == &*args[1]) {
#ifdef DEBUG
        cout << "self-set\n";
#endif
    } else {
        visit([&args](auto && w) { *args[0] = w; }, *args[1]);
    }
    return make_shared<Var>(VarVoid{});
}

EnvEntry f_setj(span<EnvEntry> args)
{
    if (args.size() != 2)
        throw RunError("set! requires 2 args");
    if (args[0]->index() != args[1]->index()) {
#ifdef DEBUG
        cout << "set! to different type\n";
#endif
    }
    return setjj(args);
}

EnvEntry f_setjj(span<EnvEntry> args)
{
    if (args.size() != 2)
        throw RunError("set!! requires 2 args");
    return setjj(args);
}

} // ans

namespace humble {

Names init_env()
{
    Names n{              /* note: ordered */
            "=>",         /* NAM_THEN = 0  */
            "else",       /* NAM_ .. etc   */
            "quote",      /* as detail.hpp */
            "quasiquote",
            "unquote",
            "macro",
            "car",
            "eqv?",
            "list",
            "nonlist",
            "set!!",
            "dup",
            "error",
            "splice",
    };

    auto & g = GlobalEnv::instance();
    g.set(n.intern("list"), make_shared<Var>(VarFunHost{ f_list }));
    g.set(n.intern("nonlist"), make_shared<Var>(VarFunHost{ f_nonlist }));
    g.set(n.intern("+"), make_shared<Var>(VarFunHost{ f_pluss }));
    g.set(n.intern("set!"), make_shared<Var>(VarFunHost{ f_setj }));
    g.set(n.intern("set!!"), make_shared<Var>(VarFunHost{ f_setjj }));

    return n;
}

} // ns

