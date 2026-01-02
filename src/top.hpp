#ifndef HUMBLE_TOP
#define HUMBLE_TOP

#include "vars.hpp"
#include "macros.hpp"
#include "tok.hpp"

#include <tuple>
#include <iosfwd>

namespace humble {

std::tuple<Names, GlobalEnv, Macros> init_top(SrcOpener * src_opener);
void print(EnvEntry a, Names & n, std::ostream & os);

} // ns

#endif
