#ifndef HUMBLE_CONS
#define HUMBLE_CONS

#include "vars.hpp"

namespace humble {

using ConsPtr = std::shared_ptr<Cons>;
using ConsNext = std::variant<ConsPtr, EnvEntry>;

struct Cons {
    EnvEntry a;
    ConsNext d;  // invariant: when EnvEntry not VarCons

    Cons(EnvEntry a, ConsNext d);
    VarCons xcopy(size_t n);
    std::variant<VarList, VarNonlist> to_list_var();
    size_t length();
    static VarCons from_list(std::span<EnvEntry> x);
    static VarCons from_nonlist(std::span<EnvEntry> x);
    static Cons * last;
};

ConsPtr to_cons(Var & x);
ConsPtr to_cons_copy(Var & x);
VarList normal_list(Var & x);

struct ConsOrListIter {
    virtual EnvEntry get() = 0;
    virtual ~ConsOrListIter();
};

std::unique_ptr<ConsOrListIter> make_iter(Var & x);

} // ns

#endif
