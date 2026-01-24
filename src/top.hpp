#ifndef HUMBLE_TOP
#define HUMBLE_TOP

#include "vars.hpp"
#include "macros.hpp"
#include "tok.hpp"

#include <tuple>
#include <iosfwd>

namespace humble {

GlobalEnv init_top(Macros & macros);
void top_included(Names & names, Macros & macros);
void print(EnvEntry a, Names & n, std::ostream & os);

} // ns

#endif
