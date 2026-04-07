#ifndef HUMBLE_XDL
#define HUMBLE_XDL

#include "parse.hpp"
#include "vars.hpp"
#include <iosfwd>

namespace humble {

struct LibLoader {
    std::string libs_dir;
    std::set<std::string> requires_accum;
    LibLoader(std::string libs_dir);
    std::unique_ptr<Macro> requires_macro();
    void operator()(GlobalEnv & env, Names & n, std::ostream & es);
};

} // ns

#endif
