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

ConsPtr to_cons(Var & x)
{
    if (holds_alternative<VarCons>(x)) {
        return get<VarCons>(x).c;
    } else if (holds_alternative<VarList>(x)) {
        return Cons::from_list(get<VarList>(x).v).c;
    } else if (holds_alternative<VarNonlist>(x)) {
        return Cons::from_nonlist(get<VarNonlist>(x).v).c;
    }
    throw CoreError("to_cons on not list");
}

ConsPtr to_cons_copy(Var & x)
{
    if (holds_alternative<VarCons>(x)) {
        get<VarCons>(x).c->xcopy(0);
    }
    return to_cons(x);
}

VarList normal_list(Var & x)
{
    if (holds_alternative<VarCons>(x)) {
        ConsPtr c = get<VarCons>(x).c;
        if (not c)
            return {};
        auto r = c->to_list_var();
        if (not holds_alternative<VarList>(r))
            throw RunError("nonlist for list-use");
        return get<VarList>(r);
    }
    return get<VarList>(x);
}

struct ListIter : ConsOrListIter {
    vector<EnvEntry> & v;
    size_t i;
    ListIter(vector<EnvEntry> & v) : v(v), i() { }
    EnvEntry get() override
    {
        if (i == v.size())
            return nullptr;
        return v[i++];
    }
};

struct ConsIter : ConsOrListIter {
    Cons * c;
    ConsIter(ConsPtr c) : c(c.get()) { }
    EnvEntry get() override
    {
        if (not c)
            return nullptr;
        auto r = c;
        if (not cons_iter_next(c))
            c = nullptr;
        return r->a;
    }
};

std::unique_ptr<ConsOrListIter> make_iter(Var & x)
{
    if (holds_alternative<VarList>(x))
        return make_unique<ListIter>( get<VarList>(x).v );
    if (holds_alternative<VarCons>(x))
        return make_unique<ConsIter>( get<VarCons>(x).c );
    return {};
}

} // ns

