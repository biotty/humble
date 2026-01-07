#ifndef HUMBLE_MACROS
#define HUMBLE_MACROS

#include "parse.hpp"
#include "vars.hpp"
#include <set>

namespace humble {

struct SrcOpener {
    std::string filename;
    virtual std::string operator()(std::string name) = 0;
    virtual ~SrcOpener() = default;
};

Macros init_macros(Names & names, SrcOpener & opener);
void init_macros(Macros & macros);
Lex to_lex(EnvEntry a);
EnvEntry from_lex(const Lex & x);

} // ns

#endif
