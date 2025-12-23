#ifndef HUMBLE_CONS
#define HUMBLE_CONS

#include "vars.hpp"

namespace humble {

using ConsPtr = std::shared_ptr<Cons>;
using ConsNext = std::variant<ConsPtr, EnvEntry>;

struct Cons {
    EnvEntry a;
    ConsNext d;

    Cons(EnvEntry a, ConsNext d);
    VarCons xcopy(size_t n);
    size_t length();
    static VarCons from_list(std::span<EnvEntry> x);
    static VarCons from_nonlist(std::span<EnvEntry> x);
    static Cons * last;
};

} // ns

#endif
