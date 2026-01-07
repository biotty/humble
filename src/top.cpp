#include "top.hpp"
#include "xeval.hpp"
#include "functions.hpp"

using namespace std;

namespace humble {

GlobalEnv init_top(Names & names, Macros & macros, SrcOpener & opener)
{
    names = init_env();
    auto & g = GlobalEnv::instance();
    // TODO: and add ext from arg

    macros = init_macros(names, opener);
    // TODO: add more macros if desired
    init_macros(macros);

    return g.init();
}

void print(EnvEntry a, Names & n, std::ostream & os)
{
    print(to_lex(a), n, os);
}

} // ns

