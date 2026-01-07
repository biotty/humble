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
#include <iosfwd>

namespace humble {

extern int linenumber;

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
    std::string get(int h);
};

struct LexBeg { int par; };
struct LexEnd { int par; int line; };
struct LexDot { };
struct LexQt { };
struct LexQqt { };
struct LexUnq { };

struct LexVoid { };
struct LexNum { long long i;
    // LexNum() : i{} {};
    // LexNum(long long i) : i(i) { std::cout << "INIT\n"; };
    // LexNum(const LexNum&rhs) { i=rhs.i; std::cout << "COPY\n"; }
    // LexNum&operator=(const LexNum&rhs) { i=rhs.i; std::cout << "ASSIGN\n"; return *this; }
};
struct LexBool { bool b; };
struct LexSym { int h; };
struct LexNam { int h; int line; };
struct LexString { std::string s; };
struct LexOp { int code; };
enum {
    /* APPLY when name (no op-code) */
    OP_BIND = 1,
    OP_COND,
    OP_LAMBDA,
    OP_LAMBDA_DOT,
    OP_SEQ,
    OP_IMPORT,
    OP_EXPORT,
};
struct LexImport { std::vector<int> a, b; };

struct LexEnv;  // from compx for efficient activation records
struct LexSplice;
struct LexForm;
struct LexList;
struct LexNonlist;
struct LexQuote;
struct LexQuasiquote;
struct LexUnquote;
using LexArgs = std::vector<int>;  // for fun parms and (sorted) capture
using Lex = std::variant<
    LexBeg/*0*/, LexEnd, LexQt, LexQqt, LexUnq, LexDot, LexSplice/*6*/,
    LexVoid/*7*/, LexSym, LexNum, LexBool, LexNam, LexString/*12*/,
    LexList/*13*/, LexNonlist, LexForm, LexQuote, LexQuasiquote, LexUnquote/*18*/,
    LexArgs/*19*/, LexEnv *, LexOp, LexImport/*22*/>;
struct LexSplice { std::vector<Lex> v; };
struct LexForm { std::vector<Lex> v; };
struct LexList { std::vector<Lex> v; };
struct LexNonlist { std::vector<Lex> v; };
struct LexQuote { std::vector<Lex> y; };
struct LexQuasiquote { std::vector<Lex> y; };
struct LexUnquote { std::vector<Lex> y; };

std::vector<Lex> lex(const std::string & s, Names & names);
void print(const Lex & x, Names & n, std::ostream & os);

} // ns

#endif
