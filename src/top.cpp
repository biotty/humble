#include "top.hpp"
#include "xeval.hpp"
#include "functions.hpp"

using namespace std;

namespace humble {

tuple<Names, GlobalEnv, Macros> init_top(SrcOpener * opener)
{
    auto & g = GlobalEnv::instance();

    Names names = init_env();
    // TODO: and add ext from arg
    auto m = init_macros(names, opener);
    return { names, g.init(), move(m) };
}

void print(EnvEntry a, Names & n, std::ostream & os)
{
    print(to_lex(a), n, os);
}

} // ns

