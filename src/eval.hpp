#ifndef HUMBLE_XEVAL
#define HUMBLE_XEVAL

#include "vars.hpp"
#include "tok.hpp"
#include <vector>

namespace humble {

EnvEntry run(Lex & x, Env & env, LexEnv * le);
EnvEntry xapply(std::vector<EnvEntry> v);
EnvEntry fun_call(std::vector<EnvEntry> v);

} // ns

#endif
