#ifndef HUMBLE_PARSE
#define HUMBLE_PARSE

#include "tok.hpp"
#include <memory>

namespace humble {

struct Macro { virtual Lex operator()(LexForm & t) = 0; };
using Macros = std::map<int, std::unique_ptr<Macro>>;

Names init_names();  // known names by code-point
Macros qt_macros();  // macros needed as part of "parsing"

LexForm parse(const std::string & s, Names & m, Macros & macros);
void expand_macros(Lex & t, Macros & macros, int qq);

struct Quote : Macro { Lex operator()(LexForm & t); };
struct Quasiquote : Macro { Lex operator()(LexForm & t); };
struct Unquote : Macro { Lex operator()(LexForm & t); };

// delegated from parsing to derive from in macro macro
struct UserMacro : Macro { };

} // ns

#endif
