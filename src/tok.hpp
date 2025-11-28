#ifndef HUMBLE_TOK
#define HUMBLE_TOK
#include <string>
#include <string_view>
#include <vector>
#include <initializer_list>
#include <functional>
#include <utility>
#include <variant>
#include <map>

namespace humble {

size_t spaces(const char * s, size_t n);

std::pair<std::string_view, size_t> tok(std::string_view s);

std::string unescape_string(std::string_view s);

class Names {
    std::vector<std::string> v;
    std::multimap<size_t, int> m;
    std::hash<std::string_view> hasher;
    int add(std::string_view name, size_t h);
public:
    Names();
    Names(std::initializer_list<std::string> w);
    size_t size();
    int intern(std::string_view name);
};

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
struct LexNam { int h; int line; };

struct LexSplice;
struct LexForm;
using Lex = std::variant<
    LexBeg, LexEnd, LexNum, LexBool, LexVoid, LexString,
    LexDot, LexSplice, LexQt, LexQqt, LexUnq, LexNam,
    LexForm>;
struct LexSplice { std::vector<Lex> v; };
struct LexForm { std::vector<Lex> v;  };

std::vector<Lex> lex(const std::string & s, Names & names);

} // ns

#endif
