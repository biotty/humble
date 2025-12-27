#include "cons.hpp"
#include "except.hpp"

using namespace humble;
using namespace std;

namespace {

bool cons_iter_next(Cons *& cur)
{
    if (not holds_alternative<ConsPtr>(cur->d))
        throw CoreError("list cdr not cons");
    if (not get<ConsPtr>(cur->d))
        return false;
    cur = &*get<ConsPtr>(cur->d);
    return true;
}

} // ans

namespace humble {

Cons::Cons(EnvEntry a, ConsNext d) : a(a), d(d) { }

VarCons Cons::xcopy(size_t n)
{
    auto cur = this;
    auto r = make_shared<Cons>(cur->a, ConsPtr{});
    auto cpy = r;
    while (cons_iter_next(cur)) {
        cpy->d = make_shared<Cons>(cur->a, ConsPtr{});
        cpy = get<ConsPtr>(cpy->d);
        if (--n == 0)
            break;
    }
    if (n)
        last = &*cpy;
    return { r };
}

size_t Cons::length()
{
    size_t r = 1;
    for (auto cur = this; cons_iter_next(cur); ++r)
        ;
    return r;
}

variant<VarList, VarNonlist> Cons::to_list_var()
{
    vector<EnvEntry> r;
    auto cur = this;
    do {
        r.push_back(cur->a);
        if (not holds_alternative<ConsPtr>(cur->d)) {
            r.push_back(get<EnvEntry>(cur->d));
            return VarNonlist{r};
        }
    } while (cons_iter_next(cur));
    return VarList{r};
}

VarCons Cons::from_list(span<EnvEntry> x)
{
    if (x.empty())
        return { nullptr };
    auto r = make_shared<Cons>(Cons{x.back(), ConsPtr{}});
    last = &*r;
    auto it = x.rbegin();
    for (++it; it != x.rend(); ++it)
        r = make_shared<Cons>(Cons{*it, r});
    return { r };
}

VarCons Cons::from_nonlist(span<EnvEntry> x)
{
    if (x.size() <= 2)
        throw CoreError("short nonlist");
    last = nullptr;
    ConsNext r = x.back();
    auto it = x.rbegin();
    for (++it; it != x.rend(); ++it)
        r = make_shared<Cons>(Cons{*it, r});
    return { get<ConsPtr>(r) };
}

Cons * Cons::last;

} // ns

