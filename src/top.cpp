#include "top.hpp"

using namespace std;

namespace humble {

tuple<Names, GlobalEnv, Macros> init_top()
{
    auto & i_env = GlobalEnv::instance();
    // TODO:  assert i_env is empty, checking keys()

    Names names;
    // TODO:  defined in functions.hpp
    // init_env(i_env, names);

    // TODO: * take arg w extra to also register in i_env
    //       * make a complete copy of env to give caller and below.

    return {
        names,
        i_env/* <-- v-- for now.  TODO: a copy */,
        init_macros(i_env.keys(), names)
    };
}

} // ns

