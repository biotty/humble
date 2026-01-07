#ifndef HUMBLE_TOP
#define HUMBLE_TOP

#include "vars.hpp"
#include "macros.hpp"
#include "tok.hpp"

#include <tuple>
#include <iosfwd>

namespace humble {

GlobalEnv init_top(Names & names, Macros & macros, SrcOpener & opener);
void print(EnvEntry a, Names & n, std::ostream & os);

} // ns

#endif
