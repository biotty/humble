#include "functions.hpp"
#include "vars.hpp"
#include "cons.hpp"
#include "detail.hpp"

using namespace humble;
using namespace std;

namespace {

EnvEntry f_list(span<EnvEntry> args)
{
    return make_shared<Var>(VarList{ { args.begin(), args.end() } });
}

EnvEntry f_nonlist(span<EnvEntry> args)
{
    return make_shared<Var>(VarNonlist{ { args.begin(), args.end() } });
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

    return n;
}

} // ns

