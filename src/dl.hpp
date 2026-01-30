#ifndef HUMBLE_DL
#define HUMBLE_DL

#include "tok.hpp"
#include "vars.hpp"

struct dl_arg {
    humble::Names * names;
    humble::GlobalEnv * env;
};
extern "C" typedef void (* dl_fn)(void *);

#endif
