#include "top.hpp"
#include "xeval.hpp"

using namespace std;

namespace humble {

tuple<Names, GlobalEnv, Macros> init_top(SrcOpener * opener)
{
    auto & g = GlobalEnv::instance();

    Names names;
    // TODO: (functions.hpp)
    // init_env(names); // and add ext from arg
    auto m = init_macros(names, opener);
    return { names, g.init(), move(m) };
}

} // ns

