#ifndef HUMBLE_MACROS
#define HUMBLE_MACROS

#include "parse.hpp"
#include <set>

namespace humble {

Macros init_macros(std::set<int> env_keys, Names & names);

} // ns

#endif
