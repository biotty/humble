#ifndef HUMBLE_TRACE
#define HUMBLE_TRACE

#include "tok.hpp"
#include <iosfwd>

namespace humble {

void trace_reset();
bool trace_put(Lex & x, LexEnv * le);
void trace_print_last(std::ostream & os);
void trace_print_all(std::ostream & os);

} // ns

#endif
