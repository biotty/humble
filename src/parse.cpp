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
        return w.v[0];
    };
    LexForm fr;
    auto & r = fr.v;
    while (i != z.size()) {
        auto x = z[i];
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

LexForm parse_i(const std::string & s, Names & names)
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
    // to-do: expand_macros
    // then (DEBUG) cout << "tree:" << ast << "\n";
    return w;
}

} // ns

