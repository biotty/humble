#include "debug.hpp"

#include "compx.hpp"
#include <array>

using namespace humble;
using namespace std;

namespace {

void out(ostream & os, const LexBeg & x) { os << x.par; }
void out(ostream & os, const LexEnd & x) { os << x.par; }
void out(ostream & os, const LexNum & x) { os << x.i; }
void out(ostream & os, const LexBool & x) { os << boolalpha << x.b; }
void out(ostream &, const LexVoid &) { }
void out(ostream & os, const LexString & x) { os << x.s; }
void out(ostream &, const LexDot &) { }
void out(ostream & os, const LexSplice & x);
void out(ostream &, const LexQt &) { }
void out(ostream &, const LexQqt &) { }
void out(ostream &, const LexUnq &) { }
void out(ostream & os, const LexNam & x) { os << x.h << ", " << x.line; }
void out(ostream & os, const LexSym & x) { os << x.h; }
void out(ostream & os, const LexForm & x);
void out(ostream & os, const LexList & x);
void out(ostream & os, const LexNonlist & x);
void out(ostream & os, const LexQuote & x);
void out(ostream & os, const LexQuasiquote & x);
void out(ostream & os, const LexUnquote & x);
void out(ostream & os, const LexArgs & x);
void out(ostream & os, const LexEnv * const & x);
void out(ostream & os, const LexOp & x) { os << x.code; };
void out(ostream & os, const LexImport & x) { out(os, x.a); os << ", "; out(os, x.b); }

} // ans

namespace humble {

ostream & operator<<(ostream & os, const Glyph & g)
{
    return os << g.u;
}

ostream & operator<<(ostream & os, const Lex & x)
{
    array<string, 23> tn = {
    "Beg", "End", "Qt", "Qqt", "Unq", "Dot", "Splice",
    "Void", "Sym", "Num", "Bool", "Nam", "String",
    "List", "Nonlist", "Form", "Quote", "Quasiquote", "Unquote",
    "Args", "Env~", "Op", "Import" };
    os << "Lex" << tn.at(x.index()) << "{";
    visit([&os](auto && arg) { out(os, arg); }, x);
    return os << "}";
}

ostream & operator<<(ostream & os, const vector<Lex> & v)
{
    if (v.empty()) return os;
    os << "{" << v.front();
    for (auto it = v.begin() + 1; it != v.end(); ++it) {
        os << ", " << *it;
    }
    return os << "}";
}

} // ns

namespace {

using namespace humble;

void out(ostream & os, const LexSplice & x) { os << x.v; }
void out(ostream & os, const LexForm & x) { os << x.v; }
void out(ostream & os, const LexList & x) { os << x.v; }
void out(ostream & os, const LexNonlist & x) { os << x.v; }
void out(ostream & os, const LexQuote & x) { os << x.y; }
void out(ostream & os, const LexQuasiquote & x) { os << x.y; }
void out(ostream & os, const LexUnquote & x) { os << x.y; }
void out(ostream & os, const LexArgs & v)
{
    if (v.empty()) {
        os << "{}";
        return;
    }
    os << "{" << v.front();
    for (auto it = v.begin() + 1; it != v.end(); ++it) {
        os << ", " << *it;
    }
    os << "}";
}
void out(ostream & os, const LexEnv * const & e) {
    out(os, e->parms());
    os << ", ";
    out(os, e->capture());
}

} // ans

