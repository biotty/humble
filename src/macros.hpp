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

void init_macros(Macros & macros, Names & names, SrcOpener & opener);
void macros_init(Macros & macros);

} // ns

#endif
