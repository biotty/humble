#ifndef HUMBLE_TOK
#define HUMBLE_TOK
#include <string>
#include <string_view>
#include <vector>
#include <utility>
#include <variant>

namespace humble {

size_t spaces(const char * s, size_t n);

std::pair<std::string_view, size_t> tok(std::string_view s);

std::string unescape_string(std::string_view s);

using Names = std::vector<std::string>;

size_t intern(std::string_view name, Names & names);

struct LexBeg { int par; };
struct LexEnd { int par; int line; };
struct LexNum { long long i; };
struct LexBool { bool b; };
struct LexVoid { };
struct LexString { std::string s; };
struct LexDot { };
struct LexQt { };
struct LexQqt { };
struct LexUnq { };
struct LexName { unsigned h; int line; };

struct LexSplice;
struct LexForm;
using Lex = std::variant<
    LexBeg, LexEnd, LexNum, LexBool, LexVoid, LexString,
    LexDot, LexSplice, LexQt, LexQqt, LexUnq, LexName,
    LexForm>;
struct LexSplice { std::vector<Lex> v; };
struct LexForm { std::vector<Lex> v;  };

std::vector<Lex> lex(const std::string & s, Names & names);

} // ns
#endif
