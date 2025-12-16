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
    std::string get(int h);
};

struct LexBeg { int par; };
struct LexEnd { int par; int line; };
struct LexDot { };
struct LexQt { };
struct LexQqt { };
struct LexUnq { };

struct LexVoid { };
struct LexNum { long long i; };
struct LexBool { bool b; };
struct LexSym { int h; };
struct LexNam { int h; int line; };
struct LexString { std::string s; };
struct LexOp { int code; };
enum {
    OP_DEFINE = 1,
    OP_COND,
    OP_SEQ,
    OP_IMPORT,
    OP_EXPORT,
    OP_LAMBDA,
    OP_LAMBDA_DOT,
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
    LexBeg, LexEnd, LexQt, LexQqt, LexUnq, LexDot, LexSplice,
    LexVoid, LexSym, LexNum, LexBool, LexNam, LexString,
    LexList, LexNonlist, LexForm, LexQuote, LexQuasiquote, LexUnquote,
    LexArgs, LexEnv *, LexOp, LexImport>;
struct LexSplice { std::vector<Lex> v; };
struct LexForm { std::vector<Lex> v; };
struct LexList { std::vector<Lex> v; };
struct LexNonlist { std::vector<Lex> v; };
struct LexQuote { std::vector<Lex> y; };
struct LexQuasiquote { std::vector<Lex> y; };
struct LexUnquote { std::vector<Lex> y; };

std::vector<Lex> lex(const std::string & s, Names & names);

} // ns

#endif
