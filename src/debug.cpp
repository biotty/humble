#include "debug.hpp"
#include <array>

using namespace std;

namespace {

using namespace humble;

void out(ostream & os, LexBeg & x) { os << x.par; }
void out(ostream & os, LexEnd & x) { os << x.par; }
void out(ostream & os, LexNum & x) { os << x.i; }
void out(ostream & os, LexBool & x) { os << boolalpha << x.b; }
void out(ostream &, LexVoid &) { }
void out(ostream & os, LexString & x) { os << x.s; }
void out(ostream &, LexDot &) { }
void out(ostream & os, LexSplice & x);
void out(ostream &, LexQt &) { }
void out(ostream &, LexQqt &) { }
void out(ostream &, LexUnq &) { }
void out(ostream & os, LexNam & x) { os << x.h << ", " << x.line; }
void out(ostream & os, LexSym & x) { os << x.h; }
void out(ostream & os, LexForm & x);
void out(ostream & os, LexList & x);
void out(ostream & os, LexNonlist & x);
void out(ostream & os, LexQuote & x);
void out(ostream & os, LexQuasiquote & x);
void out(ostream & os, LexUnquote & x);

} // ans

namespace humble {

ostream & operator<<(ostream & os, Glyph & g)
{
    return os << g.u;
}

ostream & operator<<(ostream & os, Lex & x)
{
    array<string, 19> tn = {
        "Beg", "End", "Num", "Bool", "Void", "String",
        "Dot", "Splice", "Qt", "Qqt", "Unq", "Nam", "Sym",
        "Form", "List", "Nonlist", "Quote", "Quasiquote", "Unquote" };
    os << "Lex" << tn.at(x.index()) << "{";
    visit([&os](auto arg) { out(os, arg); }, x);
    return os << "}";
}

ostream & operator<<(ostream & os, vector<Lex> & v)
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

void out(ostream & os, LexSplice & x) { os << x.v; }
void out(ostream & os, LexForm & x) { os << x.v; }
void out(ostream & os, LexList & x) { os << x.v; }
void out(ostream & os, LexNonlist & x) { os << x.v; }
void out(ostream & os, LexQuote & x) { os << x.y; }
void out(ostream & os, LexQuasiquote & x) { os << x.y; }
void out(ostream & os, LexUnquote & x) { os << x.y; }

} // ans

