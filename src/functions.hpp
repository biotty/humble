#ifndef HUMBLE_FUNCTIONS
#define HUMBLE_FUNCTIONS

#include "tok.hpp"
#include "vars.hpp"
#include <iosfwd>

namespace humble {

void init_env(Names & names);
void print(EnvEntry a, Names & n, std::ostream & os);

} // ns

#endif
