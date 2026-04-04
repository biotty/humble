#ifndef HUMBLE_TOP
#define HUMBLE_TOP

#include "vars.hpp"
#include "macros.hpp"
#include "tok.hpp"
#include <iosfwd>

namespace humble {

struct Opener : SrcOpener {
    constexpr static struct noresolve_t { } noresolve { };
    std::string src_dir;
    Opener(std::string src_dir);
    std::string operator()(std::string name) override;
    std::string operator()(std::string name, noresolve_t);
};

struct LibLoader {
    std::string libs_dir;
    std::set<std::string> requires_accum;
    LibLoader(std::string libs_dir);
    std::unique_ptr<Macro> requires_macro();
    void operator()(GlobalEnv & env, Names & n, std::ostream & es);
};

void top_included(Names & names, Macros & macros, std::vector<LexEnv *> & local_envs);

} // ns

#endif
