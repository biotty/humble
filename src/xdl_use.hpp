#ifndef HUMBLE_XDL_USE
#define HUMBLE_XDL_USE

#include "tok.hpp"
#include "vars.hpp"

struct xdl_arg {
    humble::Names * names;
    humble::GlobalEnv * env;
};
extern "C" typedef void (* xdl_fn)(void *);

#endif
