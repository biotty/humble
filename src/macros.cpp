#include "macros.hpp"
#include "vars.hpp"

#include <iostream>

using namespace std;

namespace humble {

Macros init_macros(Names & names, SrcOpener * opener)
{
    auto env_keys = GlobalEnv::instance().keys();
    auto m = qt_macros();

    (void)env_keys;
    (void)names;
    (void)opener;

    return m;
}

} // ns

