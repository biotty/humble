#ifndef HUMBLE_TOP
#define HUMBLE_TOP

#include "macros.hpp"  // SrcOpener here, due to the include-macro

namespace humble {

struct Opener : SrcOpener {
    constexpr static struct noresolve_t { } noresolve { };
    std::string src_dir;
    Opener(std::string src_dir);
    std::string operator()(std::string name) override;
    std::string operator()(std::string name, noresolve_t);
};

void top_included(Names & names, Macros & macros, std::vector<LexEnv *> & local_envs);

} // ns

#endif
