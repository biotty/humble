#ifndef HUMBLE_DETAIL
#define HUMBLE_DETAIL

#include "tok.hpp"
#include <span>

#define PAR_BEG "([{"
#define PAR_END ")]}"

namespace humble {

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

// parse.cpp
bool is_dotform(const LexForm & x);
LexForm without_dot(const LexForm & x);
LexForm with_dot(const LexForm & x);

// compx.cpp
std::span<Lex> span1(std::span<Lex> x, size_t i);

} // ns

#endif
