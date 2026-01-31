#include "debug.hpp"

#include "compx.hpp"
#include "fun_impl.hpp"  // for u_names
#include <array>

using namespace humble;
using namespace std;

namespace {

string op_repr(int op)
{
    switch (op) {
        case OP_BIND: return "BIND";
        case OP_COND: return "COND";
        case OP_LAMBDA: return "LAMBDA";
        case OP_LAMBDA_DOT: return "LAMBDA_DOT";
        case OP_SEQ: return "SEQ";
        case OP_IMPORT: return "IMPORT";
        case OP_EXPORT: return "EXPORT";
    }
    return "<UNKNOWN>";
}

void out(ostream & os, const LexBeg & x) { os << x.par; }
void out(ostream & os, const LexEnd & x) { os << x.par; }
void out(ostream & os, const LexNum & x) { os << x.i; }
void out(ostream & os, const LexBool & x) { os << boolalpha << x.b; }
void out(ostream &, const LexVoid &) { }
void out(ostream & os, const LexString & x) { os << '"' << x.s << '"'; }
void out(ostream &, const LexDot &) { }
void out(ostream &, const LexSpl &) { }
void out(ostream &, const LexR &) { }
void out(ostream &, const LexQt &) { }
void out(ostream &, const LexQqt &) { }
void out(ostream &, const LexUnq &) { }
void out(ostream & os, const LexNam & x) { os << u_names->get(x.h) << " L" << x.line; }
void out(ostream & os, const LexSym & x) { os << x.h; }
void out(ostream & os, const LexForm & x);
void out(ostream & os, const LexList & x);
void out(ostream & os, const LexNonlist & x);
void out(ostream & os, const LexRec & x);
void out(ostream & os, const LexQuote & x);
void out(ostream & os, const LexQuasiquote & x);
void out(ostream & os, const LexUnquote & x);
void out(ostream & os, const LexArgs & x);
void out(ostream & os, const LexEnv * const & x);
void out(ostream & os, const LexOp & x) { os << op_repr(x.code); };
void out(ostream & os, const LexImport & x) { out(os, x.a); os << ", "; out(os, x.b); }

} // ans

namespace humble {

ostream & operator<<(ostream & os, const Glyph & g)
{
    return os << g.u;
}

ostream & operator<<(ostream & os, const Lex & x)
{
    array<string, 25> tn = {  // note: ordered as Lex variants
    "Beg", "End", "Qt", "Qqt", "Unq", "Dot", "Spl", "R",
    "Void", "Sym", "Num", "Bool", "Nam", "String",
    "List", "Nonlist", "Form", "Quote", "Quasiquote", "Unquote",
    "Args", "Env~", "Op", "Import", "Rec" };
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

void out(ostream & os, const LexForm & x) { os << x.v; }
void out(ostream & os, const LexList & x) { os << x.v; }
void out(ostream & os, const LexNonlist & x) { os << x.v; }
void out(ostream & os, const LexRec & x) { os << x.v; }
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

