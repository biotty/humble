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
};

inline auto nam_quote = LexName{ NAM_QUOTE, 0 };
inline auto nam_quasiquote = LexName{ NAM_QUASIQUOTE, 0 };
inline auto nam_unquote = LexName{ NAM_UNQUOTE, 0 };

}

#endif

