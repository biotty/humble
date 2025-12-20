#include "cons.hpp"

using namespace std;

namespace humble {

Cons::Cons(ConsReg a, ConsReg d) : a(a), d(d) { }

VarCons Cons::from_list(span<EnvEntry> x)
{
    if (x.empty())
        return { nullptr };
    auto r = make_shared<Cons>(Cons{x.back(), ConsPtr{}});
    last = &*r;
    auto it = x.rbegin();
    for (++it; it != x.rend(); ++it) {
        r = make_shared<Cons>(Cons{*it, r});
    }
    return { r };
}

Cons * Cons::last;

} // ns

