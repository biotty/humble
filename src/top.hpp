#ifndef HUMBLE_TOP
#define HUMBLE_TOP

#include "vars.hpp"  // TODO: functions.hpp
#include "macros.hpp"

#include <tuple>

namespace humble {

std::tuple<Names, GlobalEnv, Macros> init_top();

} // ns

#endif
