#ifndef HUMBLE_CONS
#define HUMBLE_CONS

#include "vars.hpp"

namespace humble {

using ConsPtr = std::shared_ptr<Cons>;
using ConsReg = std::variant<EnvEntry, ConsPtr>;

struct Cons {
    ConsReg a;
    ConsReg d;

    Cons(ConsReg a, ConsReg d);
    static VarCons from_list(std::span<EnvEntry> x);
    static Cons * last;
};

} // ns

#endif
