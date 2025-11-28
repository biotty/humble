#ifndef HUMBLE_DEBUG
#define HUMBLE_DEBUG
#include "utf.hpp"
#include "tok.hpp"
#include <iostream>

// Interface provides means to output various data
// for the purpose of inspection during development.

namespace humble {

using namespace std;

ostream & operator<<(ostream & os, Glyph & g);
ostream & operator<<(ostream & os, Lex & x);
ostream & operator<<(ostream & os, vector<Lex> & v);

} // ns

#endif
