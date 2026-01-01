#ifndef HUMBLE_MACROS
#define HUMBLE_MACROS

#include "parse.hpp"
#include <set>

namespace humble {

struct SrcOpener {
    std::string filename;
    virtual std::string operator()(std::string name) = 0;
    virtual ~SrcOpener() = default;
};

Macros init_macros(Names & names, SrcOpener * opener);

} // ns

#endif
