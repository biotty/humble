#ifndef HUMBLE_XEVAL
#define HUMBLE_XEVAL

#include "vars.hpp"
#include "tok.hpp"
#include <vector>

namespace humble {

EnvEntry run(Lex & x, Env & env);
EnvEntry xapply(std::vector<EnvEntry> v);

} // ns

#endif
