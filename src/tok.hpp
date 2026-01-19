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
#include <span>
#include <iosfwd>

#define PAR_BEG "([{"
#define PAR_END ")]}"

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
struct LexSpl { };
struct LexR { };

struct LexVoid { };
struct LexNum { long long i; };
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
struct LexForm;
struct LexList;
struct LexNonlist;
struct LexRec;
struct LexQuote;
struct LexQuasiquote;
struct LexUnquote;
using LexArgs = std::vector<int>;  // for fun parms and (sorted) capture
using Lex = std::variant<
    LexBeg/*0*/, LexEnd, LexQt, LexQqt, LexUnq, LexDot, LexSpl, LexR,
    LexVoid/*8*/, LexSym, LexNum, LexBool, LexNam, LexString,
    LexList/*14*/, LexNonlist, LexForm, LexQuote, LexQuasiquote, LexUnquote,
    LexArgs/*20*/, LexEnv *, LexOp, LexImport, LexRec/*24*/>;
struct LexForm { std::vector<Lex> v;
    /* cannot do the following because i use aggregate construct syntax at callers
     * (doing the following collapses one level)
    LexForm() { std::cout << "LexForm DFLT-INIT " << this << std::endl; };
    LexForm(const std::vector<Lex> & v) : v(v) { std::cout << "LexForm INIT " << this << std::endl; };
    LexForm(const LexForm&rhs) : v(rhs.v) { std::cout << "LexForm COPY " << this << " from " << &rhs << std::endl; }
    LexForm(const LexForm&&rhs) : v(std::move(rhs.v)) { std::cout << "LexForm MOVE " << this << "from " << &rhs << std::endl; }
    LexForm&operator=(const LexForm&rhs) { v=rhs.v; std::cout << "LexForm ASSIGN " << this << " from " << &rhs << std::endl; return *this; }
    ~LexForm() { std::cout << "LexForm DELETE " << this << "\n"; }
    */
};
struct LexList { std::vector<Lex> v; };
struct LexNonlist { std::vector<Lex> v; };
struct LexRec { std::vector<Lex> v; };
struct LexQuote { std::vector<Lex> y; };
struct LexQuasiquote { std::vector<Lex> y; };
struct LexUnquote { std::vector<Lex> y; };

std::vector<Lex> lex(const std::string & s, Names & names);
void print(const Lex & x, Names & n, std::ostream & os);

std::span<Lex> span1(std::span<Lex> x, size_t i);

enum {
    NAM_THEN,
    NAM_ELSE,
    NAM_QUOTE,
    NAM_QUASIQUOTE,
    NAM_UNQUOTE,
    NAM_MACRO,
    NAM_CAR,
    NAM_EQVP,
    NAM_LIST,
    NAM_NONLIST,
    NAM_SETJJ,
    NAM_DUP,
    NAM_ERROR,
    NAM_SPLICE,
    NAM_IMPORT,
};

inline auto nam_then = LexNam{ NAM_THEN, 0 };
inline auto nam_else = LexNam{ NAM_ELSE, 0 };
inline auto nam_quote = LexNam{ NAM_QUOTE, 0 };
inline auto nam_quasiquote = LexNam{ NAM_QUASIQUOTE, 0 };
inline auto nam_unquote = LexNam{ NAM_UNQUOTE, 0 };
inline auto nam_macro =  LexNam{ NAM_MACRO, 0 };
inline auto nam_car = LexNam{ NAM_CAR, 0 };
inline auto nam_eqvp = LexNam{ NAM_EQVP, 0 };
inline auto nam_list = LexNam{ NAM_LIST, 0 };
inline auto nam_nonlist = LexNam{ NAM_NONLIST, 0 };
inline auto nam_setjj = LexNam{ NAM_SETJJ, 0 };
inline auto nam_dup = LexNam{ NAM_DUP, 0 };
inline auto nam_error = LexNam{ NAM_ERROR, 0 };
inline auto nam_splice = LexNam{ NAM_SPLICE, 0 };
inline auto nam_import = LexNam{ NAM_IMPORT, 0 };

} // ns

#endif
