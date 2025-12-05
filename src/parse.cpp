#include "parse.hpp"
#include "api.hpp"
#include "detail.hpp"
#include <sstream>
#include <tuple>

#ifdef DEBUG
#include "debug.hpp"
#endif

using namespace std;

namespace {

using namespace humble;

enum {
    PARSE_MODE_TOP = -2,  // inside no form paren
    PARSE_MODE_ONE = -1,  // yield single element
};  // otherwise 0, 1 or 2 -- offset in "([{"

// Function recurses on parens in flat list of tokens,
// to produce forms of the names and literals.  Quotes
// are represented by forms, but no (non)lists are
// produced (later, done in macro expansion).
std::pair<LexForm, size_t>
parse_r(const std::vector<Lex> & z, size_t i, int paren_mode, int depth)
{
    auto par_beg = PAR_BEG;
    auto par_end = PAR_END;
    auto parse1 = [&i, &z, depth]()
    {
        auto [w, j] = parse_r(z, i + 1, PARSE_MODE_ONE, depth);
        if (w.v.size() != 1) throw CoreError("parse one");
        i = j;
        return std::move(w.v.at(0));
    };
    LexForm fr;
    auto & r = fr.v;
    while (i != z.size()) {
        auto & x = z.at(i);
        if (std::holds_alternative<LexEnd>(x)) {
            if (get<LexEnd>(x).par != paren_mode) {
                ostringstream oss;
                oss << "parens '" << par_end[get<LexEnd>(x).par];
                oss << "' at line " << get<LexEnd>(x).line;
                oss << " does not match '";
                oss << (paren_mode >= 0 ?  string(par_beg + paren_mode, 1)
                        : "(none)") << "'";
                throw SrcError(oss.str());
            }
            return {fr, i + 1};
        } else if (std::holds_alternative<LexBeg>(x)) {
            auto [w, j] = parse_r(z, i + 1, get<LexBeg>(x).par, depth + 1);
            r.push_back(w);
            i = j;
        } else if (std::holds_alternative<LexQt>(x)) {
            r.push_back(LexForm{{nam_quote, parse1()}});
        } else if (std::holds_alternative<LexQqt>(x)) {
            r.push_back(LexForm{{nam_quasiquote, parse1()}});
        } else if (std::holds_alternative<LexUnq>(x)) {
            r.push_back(LexForm{{nam_unquote, parse1()}});
        } else if (std::holds_alternative<LexSplice>(x)) {
            r.push_back(LexSplice{{parse1()}});
        } else {
            i += 1;
            r.push_back(x);
        }
        if (paren_mode == PARSE_MODE_ONE) {
#ifdef DEBUG
            cout << "parse1: " << r << "\n";
#endif
            return { fr, i };
        }
    }
    if (paren_mode != PARSE_MODE_TOP) {
                ostringstream oss;
                oss << "parens '" << par_beg[paren_mode];
                oss << "' depth " << depth << " not closed";
        throw SrcError(oss.str());
    }
#ifdef DEBUG
    cout << "parse: " << r << "\n";
#endif
    return { fr, i };
}

} // ans

namespace humble {

Names init_names()
{
    return {
        /* NAM_THEN */ "=>",
        /* NAM_ELSE */ "else",
        /* NAM_QUOTE */ "quote",
        /* NAM_QUASIQUOTE */ "quasiquote",
        /* NAM_UNQUOTE */ "unquote",
        /* NAM_MACRO */ "macro",
        /* NAM_CAR */ "car",
        /* NAM_EQVP */ "eqv?",
        /* NAM_LIST */ "list",
        /* NAM_NONLIST */ "nonlist",
        /* NAM_SETVJJ */ "setv!!",
        /* NAM_DUP */ "dup",
        /* NAM_ERROR */ "error",
        /* NAM_SPLICE */ "splice"
    };
}

Macros qt_macros()
{
    Macros r;
    r.insert({NAM_QUOTE, make_unique<Quote>()});
    r.insert({NAM_QUASIQUOTE, make_unique<Quasiquote>()});
    r.insert({NAM_UNQUOTE, make_unique<Unquote>()});
    return r;
}

LexForm parse(const std::string & s, Names & names, Macros & macros)
{
    vector<Lex> z;
    linenumber = 1;
    try {
        z = lex(s, names);
    } catch (const SrcError & e) {
        if (filename.empty()) throw;
        ostringstream oss;
        oss << "line " << linenumber << ": " << e.what();
        throw SrcError(oss.str());
    }
    auto [w, i] = parse_r(z, 0, PARSE_MODE_TOP, 0);
    if (i != z.size()) {
        throw CoreError("not fully consumed; unexpected");
    }
    for (auto & x : w.v) {
        expand_macros(x, macros, 0);
    }
#ifdef DEBUG
    cout << "tree: " << w.v << "\n";
#endif
    return w;
/*  or, as ref.impl but i observe i cannot
    Lex r = w;
    expand_macros(r, macros, 0);
    return get<LexForm>(r); */
}

void expand_macros(Lex & t, Macros & macros, int qq)
{
    if (not holds_alternative<LexForm>(t)) return;
    auto & w = get<LexForm>(t);
    if (w.v.size() == 0) return;
    // note: I could let a Macro class hierarchy take care of
    // cases for user and quote recursion control so that
    // conditions are not stated here. That is a choice.
    bool is_user = false;
    bool is_quote = false;
    bool current = false;
    int h;
    bool is_macro = holds_alternative<LexNam>(w.v.at(0))
        and macros.contains(h = get<LexNam>(w.v.at(0)).h);
    if (is_macro) {
        is_user = dynamic_cast<UserMacro *>(macros.at(h).get());
        current = qq == 0;
        if (h == NAM_QUOTE) {
            is_quote = true;
        } else if (h == NAM_QUASIQUOTE) {
            ++qq;
        } else if (h == NAM_UNQUOTE) {
            if (--qq == 0) current = true;
        }
    }
    auto args_exp = & macros;
    if (is_user) {
        // A user-macro must observe input prior to language-macros
        // expansion, which is instead performed below, as their
        // output is not interpreter-ready as for language-macros.
        // however, the quotation macros are processed on the args
        // in addition to eventually on their output.
        static Macros user_exp;
        if (user_exp.empty()) {
            user_exp = qt_macros();
        }
        args_exp = & user_exp;
    } else if (is_quote) {
        // Normal quote arguments must not know of macros except
        // for quasi-quotation processing.
        static Macros quote_exp;
        if (quote_exp.empty()) {
            quote_exp = qt_macros();
            quote_exp.erase(NAM_QUOTE);
        }
        args_exp = & quote_exp;
    }
    for (auto & x : w.v) {
        expand_macros(x, *args_exp, qq);
    }
    if (current) {
        t = (*macros.at(h))(w);
        if (is_user) expand_macros(t, macros, qq);
    }
#ifdef DEBUG
    cout << "expand: " << t << "\n";
#endif
}

bool is_dotform(const LexForm & x)
{
    auto n = x.v.size();
    if (n < 2) return false;
    return holds_alternative<LexDot>(x.v[n - 2]);
}

LexForm without_dot(const LexForm & x)
{
    if (not is_dotform(x)) throw CoreError("no dot");
    LexForm r = x;
    auto d = r.v.size() - 2;
    r.v.erase(r.v.begin() + d);
    return r;
}

LexForm with_dot(const LexForm & x)
{
    if (is_dotform(x)) throw CoreError("has dot");
    LexForm r = x;
    auto d = r.v.size() - 1;
    r.v.insert(r.v.begin() + d, LexDot{});
    return r;
}

    template <typename T>
Lex join_nonlist(LexForm & w)
{
    auto z = get<T>(w.v.back());
    w.v.pop_back();
    for (Lex & x : z.v) {
        w.v.push_back(move(x));
    }
    z.v = move(w.v);
    return z;
}

Lex quote(Lex & t, bool quasi)
{
    if (holds_alternative<LexForm>(t)) {
        auto & f = get<LexForm>(t);
        if (is_dotform(f)) {
            LexForm w = without_dot(f);
            for (Lex & x : w.v) {
                x = quote(x, quasi);
            }
            if (holds_alternative<LexList>(w.v.back()))
                return join_nonlist<LexList>(w);
            if (holds_alternative<LexNonlist>(w.v.back()))
                return join_nonlist<LexNonlist>(w);
            return LexNonlist{w.v};
        }
        LexList r;
        for (Lex & x : f.v) {
            r.v.push_back(quote(x, quasi));
        }
        return r;
    }
    if (holds_alternative<LexNam>(t)) {
        return LexSym{get<LexNam>(t).h};
    }
    if (holds_alternative<LexSym>(t)
            || holds_alternative<LexList>(t)
            || holds_alternative<LexNonlist>(t)
            || holds_alternative<LexQuote>(t)) {
        if (quasi) return LexQuasiquote{{move(t)}};
        else return LexQuote{{move(t)}};
    }
    if (quasi) {
        if (holds_alternative<LexUnquote>(t)) {
            return move(get<LexUnquote>(t).y.at(0));
        }
    }
    return move(t);
}

void QtArgCheck(string name, LexForm & t)
{
    if (t.v.size() != 2) throw SrcError(name);
}

Lex Quote::operator()(LexForm & t)
{
    QtArgCheck("quote", t);
    return quote(t.v[1], false);
}

Lex Quasiquote::operator()(LexForm & t)
{
    QtArgCheck("quasiquote", t);
    return quote(t.v[1], true);
}

Lex Unquote::operator()(LexForm & t)
{
    QtArgCheck("unquote", t);
    auto & w = t.v[1];
    if (holds_alternative<LexForm>(w))
        return LexUnquote{{move(w)}};
    if (holds_alternative<LexList>(w))
        return LexForm{move(get<LexList>(w).v)};
    if (holds_alternative<LexNonlist>(w))
        return with_dot(LexForm{move(get<LexNonlist>(w).v)});
    if (holds_alternative<LexNam>(w) || holds_alternative<LexUnquote>(w))
        return LexUnquote{{move(w)}};
    if (holds_alternative<LexSym>(w))
        return LexNam{get<LexSym>(w).h, 0};
    return move(w);
}

} // ns

