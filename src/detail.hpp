#ifndef HUMBLE_DETAIL
#define HUMBLE_DETAIL

#include "tok.hpp"

#define PAR_BEG "([{"
#define PAR_END ")]}"

namespace humble {

extern std::string filename;
extern int linenumber;

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
    NAM_SETVJJ,
    NAM_DUP,
    NAM_ERROR,
    NAM_SPLICE,
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
inline auto nam_setvjj = LexNam{ NAM_SETVJJ, 0 };
inline auto nam_dup = LexNam{ NAM_DUP, 0 };
inline auto nam_error = LexNam{ NAM_ERROR, 0 };
inline auto nam_splice = LexNam{ NAM_SPLICE, 0 };

} // ns

#endif
