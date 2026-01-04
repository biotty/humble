#include "except.hpp"
#include "functions.hpp"
#include "vars.hpp"
#include "cons.hpp"
#include "detail.hpp"

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
    g.set(NAM_LIST, make_shared<Var>(VarFunHost{ f_list }));
    g.set(NAM_NONLIST, make_shared<Var>(VarFunHost{ f_nonlist }));
    g.set(n.intern("+"), make_shared<Var>(VarFunHost{ f_pluss }));

    return n;
}

} // ns

