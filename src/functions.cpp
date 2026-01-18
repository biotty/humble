#include "functions.hpp"
#include "except.hpp"
#include "vars.hpp"
#include "cons.hpp"
#include "xeval.hpp"
#include "compx.hpp"
#include "utf.hpp"
#include "debug.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace humble;
using namespace std;

namespace {

const char * var_type_name[] {
    "void",                     // note:  ordered as
    "number",                   // Var variant index
    "bool",
    "name",
    "string",
    "list",
    "nonlist",
    "splice",
    "unquote",
    "fun-ops",
    "fun-host",
    "lz-apply",
    "const",
};

template <typename T, typename... Ts>
bool valt_in(Var & v)
{
    if (holds_alternative<T>(v)) return true;
    if constexpr (sizeof...(Ts) != 0) return valt_in<Ts...>(v);
    return false;
}

template <typename... Ts>
void valt_or_fail(span<EnvEntry> args, size_t i, string s)
{
    if (valt_in<Ts...>(*args[i])) return;
    ostringstream ost;
    ost << s << " args[" << i << "] " << var_type_name[args[i]->index()];
    throw RunError(ost.str());
}

//
// cons, non(list)
//

EnvEntry f_list(span<EnvEntry> args)
{
    if (args.empty()) return make_shared<Var>(VarCons{});
    return make_shared<Var>(VarList{ { args.begin(), args.end() } });
}

EnvEntry f_nonlist(span<EnvEntry> args)
{
    return make_shared<Var>(VarNonlist{ { args.begin(), args.end() } });
}

EnvEntry f_list_copy(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("list-copy argc");
    valt_or_fail<VarList, VarCons>(args, 0, "list-copy");
    return make_shared<Var>(normal_list(*args[0]));
}

EnvEntry f_cons(span<EnvEntry> args)
{
    if (args.size() != 2) throw RunError("cons argc");
    if (valt_in<VarCons, VarList, VarNonlist>(*args[1])) {
        auto c = to_cons(*args[1]);
        *args[1] = VarCons{c};
        return make_shared<Var>(VarCons{make_shared<Cons>(args[0], c)});
    }
    return make_shared<Var>(VarNonlist{{args.begin(), args.end()}});
}

EnvEntry f_car(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("car argc");
    valt_or_fail<VarCons, VarList, VarNonlist>(args, 0, "car");
    Var & v = *args[0];
    if (valt_in<VarList>(v)) return get<VarList>(v).v[0];
    if (valt_in<VarNonlist>(v)) return get<VarNonlist>(v).v[0];
    auto & d = get<VarCons>(v);
    if (not d.c) throw RunError("car on null");
    return d.c->a;
}

EnvEntry c_list_ref(ConsPtr c, int i)
{
    while (i) {
        c = get<ConsPtr>(c->d);
        if (not c)
            throw RunError("list-ref cdr null");
        --i;
    }
    return c->a;
}

EnvEntry v_list_ref(const vector<EnvEntry> & v, int i)
{
    if (v.size() <= static_cast<size_t>(i))
        throw RunError("list-ref index overflow");
    return v[i];
}

EnvEntry f_list_ref(span<EnvEntry> args)
{
    if (args.size() != 2) throw RunError("list-ref argc");
    valt_or_fail<VarNum>(args, 1, "car");
    auto i = get<VarNum>(*args[1]).i;
    valt_or_fail<VarCons, VarList, VarNonlist>(args, 0, "car");
    auto & v = *args[0];
    if (valt_in<VarCons>(v))
        return c_list_ref(get<VarCons>(v).c, i);
    return v_list_ref((valt_in<VarList>(v))
            ? get<VarList>(v).v : get<VarNonlist>(v).v, i);
}

EnvEntry f_cdr(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("cdr argc");
    valt_or_fail<VarCons, VarList, VarNonlist>(args, 0, "cdr");
    Var & a = *args[0];
    if (not valt_in<VarCons>(a)) {
        if (valt_in<VarNonlist>(a)) {
            auto & u = get<VarNonlist>(a);
            if (u.v.size() <= 1)
                throw CoreError("cdr short nonlist");
            if (u.v.size() == 2)
                return u.v[1];
        }
        auto c = to_cons(a);
        a = VarCons{c};
    }
    VarCons & r = get<VarCons>(a);
    if (not r.c) throw RunError("cdr on null");
    if (holds_alternative<EnvEntry>(r.c->d))
        return get<EnvEntry>(r.c->d);
    return make_shared<Var>(VarCons{get<ConsPtr>(r.c->d)});
}

EnvEntry f_append(span<EnvEntry> args)
{
    if (args.size() == 0) return make_shared<Var>(VarCons{});
    if (args.size() == 1) return args[0];
    size_t i_last = args.size() - 1;
    auto last = args[i_last];
    if (valt_in<VarList, VarNonlist>(*last)) {
        auto c = to_cons(*last);
        *last = VarCons{c};
    }
    ConsNext p = to_cons_copy(*args[0]);
    ConsPtr r = get<ConsPtr>(p);
    ConsPtr q;
    for (size_t i = 1; i != args.size(); ++i) {
        if (holds_alternative<EnvEntry>(p)
                or get<ConsPtr>(p) != nullptr)
            q = Cons::last;
        if (i == i_last) {
            if (holds_alternative<VarCons>(*last))
                p = get<VarCons>(*last).c;
            else p = last;
        } else {
            valt_or_fail<VarCons, VarList>(args, i, "append");
            p = to_cons_copy(*args[i]);
        }
        if (q) q->d = p;
    }
    /* debug:
    cerr << "R " << r.get() << endl;
    cerr << "Q " << q.get() << endl;
    if (holds_alternative<ConsPtr>(p)) {
        cerr << "P (c) " << get<ConsPtr>(p).get() << endl;
        auto & w = get<ConsPtr>(p);
        if (holds_alternative<ConsPtr>(w->d))
            cerr << "D (c) " << get<ConsPtr>(w->d).get() << endl;
        else
            cerr << "D (v) " << get<EnvEntry>(w->d).get() << endl;
    } else {
        cerr << "P (v) " << get<EnvEntry>(p).get() << endl;
    }
    */
    return make_shared<Var>(VarCons{r});
}

EnvEntry f_set_carj(span<EnvEntry> args)
{
    if (args.size() != 2) throw RunError("set-car! argc");
    valt_or_fail<VarCons, VarList, VarNonlist>(args, 0, "set-car!");
    if (valt_in<VarCons>(*args[0])) {
        get<VarCons>(*args[0]).c->a = args[1];
    } else if (valt_in<VarList>(*args[0])) {
        get<VarList>(*args[0]).v[0] = args[1];
    } else {
        get<VarList>(*args[0]).v[0] = args[1];
    }
    return make_shared<Var>(VarVoid{});
}

EnvEntry f_set_cdrj(span<EnvEntry> args)
{
    if (args.size() != 2) throw RunError("set-cdr! argc");
    valt_or_fail<VarCons, VarList, VarNonlist>(args, 0, "set-cdr!");
    if (valt_in<VarList>(*args[1])) {
        auto c = to_cons(*args[1]);
        *args[1] = VarCons{c};
    } else if (valt_in<VarNonlist>(*args[1])) {
        auto c = to_cons(*args[1]);
        *args[1] = VarCons{c};
    }
    ConsNext d = valt_in<VarCons>(*args[1])
            ? ConsNext{get<VarCons>(*args[1]).c}
            : ConsNext{args[1]};
    if (valt_in<VarCons>(*args[0])) {
        get<VarCons>(*args[0]).c->d = d;
        return make_shared<Var>(VarVoid{});
    }
    EnvEntry a;
    if (valt_in<VarList>(*args[0])) a = get<VarList>(*args[0]).v[0];
    else if (valt_in<VarNonlist>(*args[0])) a = get<VarNonlist>(*args[0]).v[0];
    else a = args[0];
    *args[0] = VarCons{ make_shared<Cons>(a, d) };
    return make_shared<Var>(VarVoid{});
}

EnvEntry f_list_tail(span<EnvEntry> args)
{
    if (args.size() != 2) throw RunError("list-tail argc");
    valt_or_fail<VarCons, VarList>(args, 0, "list-tail");
    valt_or_fail<VarNum>(args, 1, "list-tail");
    auto n = get<VarNum>(*args[1]).i;
    ConsNext r = to_cons(*args[0]);
    *args[0] = VarCons{get<ConsPtr>(r)};
    for (auto i = 0u; i != n; ++i) {
        if (not holds_alternative<ConsPtr>(r)) {
            // warning("list-tail overrun")
            break;
        }
        r = get<ConsPtr>(r)->d;
    }
    return make_shared<Var>(VarCons{get<ConsPtr>(r)});
}

EnvEntry f_list_setj(span<EnvEntry> args)
{
    if (args.size() != 3) throw RunError("list-set! argc");
    valt_or_fail<VarCons, VarList>(args, 0, "list-set!");
    valt_or_fail<VarNum>(args, 1, "list-set!");
    auto n = get<VarNum>(*args[1]).i;
    if (valt_in<VarList>(*args[0])) {
        get<VarList>(*args[0]).v[n] = args[2];
    } else {
        vector<EnvEntry> a{args[0], args[1]};
        auto k = f_list_tail(a);
        auto c = get<VarCons>(*k).c;
        if (c) c->a = args[2];
    }
    return make_shared<Var>(VarVoid{});
}

EnvEntry f_make_list(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("make-list argc");
    valt_or_fail<VarNum>(args, 0, "make-list");
    auto n = get<VarNum>(*args[0]).i;
    vector<EnvEntry> v;
    v.reserve(n);
    auto x = make_shared<Var>(VarVoid{});
    for (auto i = 0u; i != n; ++i)
        v.push_back(x);
    return make_shared<Var>(VarList{move(v)});
}

EnvEntry f_reverse(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("reverse argc");
    valt_or_fail<VarCons, VarList>(args, 0, "reverse");
    auto r = f_list_copy(args);
    auto & a = get<VarList>(*r);
    auto n = a.v.size();
    auto m = n / 2u;
    for (auto i = 0u; i != m; ++i)
        swap(a.v[i], a.v[n - i - 1]);
    return r;
}

EnvEntry f_take(span<EnvEntry> args)
{
    if (args.size() != 2) throw RunError("take argc");
    valt_or_fail<VarNum>(args, 0, "take");
    valt_or_fail<VarCons, VarList>(args, 1, "take");
    auto n = get<VarNum>(*args[0]).i;
    if (valt_in<VarList>(*args[1])) {
        auto & a = get<VarList>(*args[1]).v;
        vector<EnvEntry> r;
        r.reserve(n);
        for (auto i = 0u; i != n; ++i)
            r.push_back(a[i]);
        return make_shared<Var>(VarList{move(r)});
    }
    if (n == 0 or get<VarCons>(*args[1]).c == nullptr)
        return make_shared<Var>(VarCons{});
    return make_shared<Var>(
            get<VarCons>(*args[1]).c->xcopy(n));
}

EnvEntry f_splice(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("splice argc");
    valt_or_fail<VarCons, VarList>(args, 0, "splice");
    return make_shared<Var>(VarSplice{normal_list(*args[0]).v});
}

//
// boolean functions (one, others are macros on cond)
//

EnvEntry f_not(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("not argc");
    return make_shared<Var>(VarBool{
            valt_in<VarBool>(*args[0])
            and get<VarBool>(*args[0]).b == false});
}

//
// numbers
//

EnvEntry f_pluss(span<EnvEntry> args)
{
    long long r{};
    for (auto i = 0u; i != args.size(); ++i) {
        valt_or_fail<VarNum>(args, i, "+");
        r += get<VarNum>(*args[i]).i;
    }
    return make_shared<Var>(VarNum{ r });
}

EnvEntry f_minus(span<EnvEntry> args)
{
    valt_or_fail<VarNum>(args, 0, "-");
    auto r = get<VarNum>(*args[0]).i ;
    if (args.size() == 1)
        return make_shared<Var>(VarNum{ -r });
    int i{};
    for (auto & a : args) {
        if (&a == &args[0]) continue;
        valt_or_fail<VarNum>(args, ++i, "-");
        r -= get<VarNum>(*a).i;
    }
    return make_shared<Var>(VarNum{ r });
}

EnvEntry f_multiply(span<EnvEntry> args)
{
    long long r{1};
    int i{};
    for (auto & x : args) {
        valt_or_fail<VarNum>(args, i++, "*");
        r *= get<VarNum>(*x).i;
    }
    return make_shared<Var>(VarNum{ r });
}

pair<long long, long long> rdiv(span<EnvEntry> args, const string & fn)
{
    valt_or_fail<VarNum>(args, 0, fn);
    auto n = get<VarNum>(*args[0]).i;
    long long d{1};
    int i{};
    for (auto & x : args) {
        if (&x == &args[0]) continue;
        valt_or_fail<VarNum>(args, ++i, fn);
        d *= get<VarNum>(*x).i;
        if (d > n * 2) break;
    }
    return {n, d};
}

EnvEntry f_divide(span<EnvEntry> args)
{
    auto [n, d] = rdiv(args, "/");
    return make_shared<Var>(VarNum{ n / d });
}

EnvEntry f_div(span<EnvEntry> args)
{
    auto [n, d] = rdiv(args, "div");
    return make_shared<Var>(VarNonlist{{
            make_shared<Var>(VarNum{ n / d }),
            make_shared<Var>(VarNum{ n % d })}});
}

typedef void (*isubj_t)(long long & i, long long j);
void isubj_max(long long & i, long long j) { if (j > i) i = j; }
void isubj_min(long long & i, long long j) { if (j < i) i = j; }

EnvEntry fold_isubj(span<EnvEntry> args, const string & fn, isubj_t f)
{
    valt_or_fail<VarNum>(args, 0, fn);
    auto r = get<VarNum>(*args[0]).i;
    int i{};
    for (auto & x : args) {
        if (&x == &args[0]) continue;
        valt_or_fail<VarNum>(args, ++i, fn);
        f(r, get<VarNum>(*x).i);
    }
    return make_shared<Var>(VarNum{ r });
}

EnvEntry f_max(span<EnvEntry> args)
{
    return fold_isubj(args, "max", isubj_max);
}

EnvEntry f_min(span<EnvEntry> args)
{
    return fold_isubj(args, "min", isubj_min);
}

EnvEntry f_abs(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("abs argc");
    valt_or_fail<VarNum>(args, 0, "abs");
    return make_shared<Var>(VarNum{abs(get<VarNum>(*args[0]).i)});
}

EnvEntry n1_pred(span<EnvEntry> args, string fn, bool(*p)(long long))
{
    if (args.size() != 1) throw RunError(fn + "argc");
    valt_or_fail<VarNum>(args, 0, fn);
    return make_shared<Var>(VarBool{ p(get<VarNum>(*args[0]).i) });
}

bool is_zero(long long i) { return i == 0; }
EnvEntry f_zerop(span<EnvEntry> args)
{ return n1_pred(args, "zero?", is_zero); }

bool is_positive(long long i) { return i > 0; }
EnvEntry f_positivep(span<EnvEntry> args)
{ return n1_pred(args, "positive?", is_positive); }

bool is_negative(long long i) { return i < 0; }
EnvEntry f_negativep(span<EnvEntry> args)
{ return n1_pred(args, "negative?", is_negative); }

bool is_even(long long i) { return not (i & 1); }
EnvEntry f_evenp(span<EnvEntry> args)
{ return n1_pred(args, "even?", is_even); }

bool is_odd(long long i) { return i == 0; }
EnvEntry f_oddp(span<EnvEntry> args)
{ return n1_pred(args, "odd?", is_odd); }

EnvEntry n2_pred(span<EnvEntry> args, string fn,
        bool(*p)(long long, long long))
{
    auto r = make_shared<Var>(VarBool{ true });
    if (args.size() == 0) return r;
    valt_or_fail<VarNum>(args, 0, fn);
    if (args.size() == 1) {
        return r;
    }
    size_t i{};
    auto j = get<VarNum>(*args[0]).i;
    for (auto & x : args) {
        if (&x == &args[0]) continue;
        valt_or_fail<VarNum>(args, ++i, fn);
        auto k = get<VarNum>(*x).i;
        if (not p(j, k)) {
            get<VarBool>(*r).b = false;
            break;
        }
        j = k;
    }
    return r;
}

bool is_eq(long long i, long long j) { return i == j; }
EnvEntry f_eq(span<EnvEntry> args)
{ return n2_pred(args, "=", is_eq); }

bool is_lt(long long i, long long j) { return i < j; }
EnvEntry f_lt(span<EnvEntry> args)
{ return n2_pred(args, "<", is_lt); }

bool is_gt(long long i, long long j) { return i > j; }
EnvEntry f_gt(span<EnvEntry> args)
{ return n2_pred(args, ">", is_gt); }

bool is_lte(long long i, long long j) { return i <= j; }
EnvEntry f_lte(span<EnvEntry> args)
{ return n2_pred(args, "<=", is_lte); }

bool is_gte(long long i, long long j) { return i >= j; }
EnvEntry f_gte(span<EnvEntry> args)
{ return n2_pred(args, ">=", is_gte); }

//
// mutation
//

EnvEntry setjj(span<EnvEntry> args)
{
    if (&*args[0] == &*args[1]) {
        warn("self-set", args);
    } else {
        visit([&args](auto && w) { *args[0] = w; }, *args[1]);
    }
    return make_shared<Var>(VarVoid{});
}

EnvEntry f_setj(span<EnvEntry> args)
{
    if (args.size() != 2) throw RunError("set! argc");
    if (args[0]->index() != args[1]->index()) {
        warn("set! to different type", args);
    }
    return setjj(args);
}

EnvEntry f_setjj(span<EnvEntry> args)
{
    if (args.size() != 2) throw RunError("set!! argc");
    return setjj(args);
}

//
// identity
//

EnvEntry f_aliasp(span<EnvEntry> args)
{
    if (args.size() != 2) throw RunError("alias? argc");
    return make_shared<Var>(VarBool{&*args[0] == &*args[1]});
}

EnvEntry f_eqp(span<EnvEntry> args)
{
    if (args.size() != 2) throw RunError("eq? argc");
    Var & a = *args[0];
    Var & b = *args[1];
    bool r{};
    if (args[0]->index() != args[1]->index()) {
        // false.  different types
    } else if (valt_in<VarNum>(a)) {
        r = (get<VarNum>(a).i == get<VarNum>(b).i);
    } else if (valt_in<VarString>(a)) {
        r = (get<VarString>(a).s == get<VarString>(b).s);
    } else if (valt_in<VarBool>(a)) {
        r = (get<VarBool>(a).b == get<VarBool>(b).b);
    } else if (valt_in<VarNam>(a)) {
        r = (get<VarNam>(a).h == get<VarNam>(b).h);
    } else if (valt_in<VarCons>(a)) {
        r = (get<VarCons>(a).c == get<VarCons>(b).c);
    } else {
        r = (&a == &b);
    }
    return make_shared<Var>(VarBool{r});
}

EnvEntry f_equalp(span<EnvEntry> args)
{
    if (args.size() != 2) throw RunError("equal? argc");
    Var & a = *args[0];
    Var & b = *args[1];
    if (not valt_in<VarList, VarNonlist, VarCons>(a)
            or not valt_in<VarList, VarNonlist, VarCons>(b))
        return f_eqp(args);
    auto r = make_shared<Var>(VarBool{});
    if ((valt_in<VarList>(a) and valt_in<VarList>(b))
            or (valt_in<VarNonlist>(a) and valt_in<VarNonlist>(b))) {
        vector<EnvEntry> * x{};
        vector<EnvEntry> * y{};
        if (valt_in<VarList>(a)) {
            x = &get<VarList>(a).v;
            y = &get<VarList>(b).v;
        } else {
            x = &get<VarNonlist>(a).v;
            y = &get<VarNonlist>(b).v;
        }
        if (x->size() != y->size()) return r;
        for (size_t i{}; i != x->size(); ++i) {
            vector<EnvEntry> q{(*x)[i], (*y)[i]};
            auto e = f_equalp(q);
            if (not get<VarBool>(*e).b) return e;
        }
        get<VarBool>(*r).b = true;
        return r;
    }
    if (valt_in<VarCons>(a) and valt_in<VarCons>(b)) {
        ConsNext x = get<VarCons>(a).c;
        ConsNext y = get<VarCons>(b).c;
        for (;;) {
            if (not holds_alternative<ConsPtr>(x)) {
                if (holds_alternative<ConsPtr>(y)) return r;
                vector<EnvEntry> q{get<EnvEntry>(x), get<EnvEntry>(y)};
                return f_equalp(q);
            }
            if (nullptr == get<ConsPtr>(x)) break;
            if (not holds_alternative<ConsPtr>(y) or nullptr == get<ConsPtr>(y))
                return r;
            vector<EnvEntry> q{get<ConsPtr>(x)->a, get<ConsPtr>(y)->a};
            auto e = f_equalp(q);
            if (not get<VarBool>(*e).b) return e;
            x = get<ConsPtr>(x)->d;
            y = get<ConsPtr>(y)->d;
        }
        get<VarBool>(*r).b = nullptr == get<ConsPtr>(y);
        return r;
    }
    if (valt_in<VarCons>(a) or valt_in<VarCons>(b)) {
        bool is_nonlist{};
        vector<EnvEntry> * v;
        ConsNext c;
        if (valt_in<VarCons>(b)) {
            v = (is_nonlist = valt_in<VarNonlist>(a))
                ? &get<VarNonlist>(a).v : &get<VarList>(a).v;
            c = get<VarCons>(b).c;
        } else {
            v = (is_nonlist = valt_in<VarNonlist>(b))
                ? &get<VarNonlist>(b).v : &get<VarList>(b).v;
            c = get<VarCons>(a).c;
        }
        size_t n = v->size();
        size_t i{};
        for (; nullptr != get<ConsPtr>(c); ++i) {
            if (i == n) return r;
            if (not holds_alternative<ConsPtr>(c)) {
                if (not is_nonlist or i + 1 != n)
                    return r;
                vector<EnvEntry> q{get<EnvEntry>(c), (*v)[i]};
                return f_equalp(q);
            }
            vector<EnvEntry> q{get<ConsPtr>(c)->a, (*v)[i]};
            auto e = f_equalp(q);
            if (not get<VarBool>(*e).b) return e;
            c = get<ConsPtr>(c)->d;
        }
        get<VarBool>(*r).b = i == n;
        return r;
    }
    // note: must now be NONLIST and LIST -- hence not equal
    return r;
}

//
// REC
//

//
// string
//

EnvEntry f_string_ref(span<EnvEntry> args)
{
    if (args.size() != 2) throw RunError("string->ref argc");
    valt_or_fail<VarString>(args, 0, "string->ref");
    valt_or_fail<VarNum>(args, 1, "string->ref");
    auto s = get<VarString>(*args[0]).s;
    auto i = get<VarNum>(*args[1]).i;
    auto w = utf_ref(s, i);
    long long r{};
    if (not w.u.empty())
        r = utf_value(w);
    return make_shared<Var>(VarNum{r});
}

EnvEntry f_string_z_list(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("string->list argc");
    valt_or_fail<VarString>(args, 0, "string->list");
    auto s = get<VarString>(*args[0]).s;
    vector<EnvEntry> v;
    for (int i = 0 ;; ++i) {
        auto w = utf_ref(s, i);
        if (w.u.empty()) break;
        auto c = utf_value(w);
        v.push_back(make_shared<Var>(VarNum{c}));
    }
    return make_shared<Var>(VarList{move(v)});
}

EnvEntry f_list_z_string(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("list->string argc");
    valt_or_fail<VarCons, VarList>(args, 0, "list->string");
    auto j = make_iter(*args[0]);
    string s;
    for (;;) {
        auto x = j->get();
        if (not x) break;
        if (not valt_in<VarNum>(*x))
            throw RunError("list->string not number");
        s += utf_make(get<VarNum>(*x).i);
    }
    return make_shared<Var>(VarString{move(s)});
}

Names * u_names;

EnvEntry f_symbol_z_string(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("symbol->string argc");
    valt_or_fail<VarNam>(args, 0, "symbol->string");
    int h = get<VarNam>(*args[0]).h;
    return make_shared<Var>(VarString{u_names->get(h)});
}

EnvEntry f_substring(span<EnvEntry> args)
{
    if (args.size() < 2) throw RunError("substring argc");
    valt_or_fail<VarString>(args, 0, "substring");
    valt_or_fail<VarNum>(args, 1, "substring");
    auto & s = get<VarString>(*args[0]).s;
    size_t i = get<VarNum>(*args[1]).i;
    auto j = s.npos;
    if (args.size() >= 3) {
        valt_or_fail<VarNum>(args, 2, "substring");
        j = get<VarNum>(*args[2]).i;
        if (j < i) j = i;
    }
    return make_shared<Var>(VarString{s.substr(i, j - i)});
}

EnvEntry f_substring_index(span<EnvEntry> args)
{
    if (args.size() != 2) throw RunError("substring-index argc");
    valt_or_fail<VarString>(args, 0, "substring-index");
    valt_or_fail<VarString>(args, 1, "substring-index");
    auto s = get<VarString>(*args[0]).s;
    auto t = get<VarString>(*args[1]).s;
    auto i = s.find(t);
    long long r = i == s.npos ? -1 : i;
    return make_shared<Var>(VarNum{r});
}

EnvEntry f_string_length(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("string-length argc");
    valt_or_fail<VarString>(args, 0, "string-length");
    auto s = get<VarString>(*args[0]).s;
    return make_shared<Var>(VarNum{static_cast<long long>(s.length())});
}

EnvEntry f_string_append(span<EnvEntry> args)
{
    string r;
    for (auto & a : args) {
        valt_or_fail<VarString>(args, 0, "string-length");
        r += get<VarString>(*a).s;
    }
    return make_shared<Var>(VarString{r});
}

typedef bool (*spred_t)(const string &, const string &);
EnvEntry spred(span<EnvEntry> args, const string & fn, spred_t f)
{
    if (args.size() != 2) throw RunError(fn);
    valt_or_fail<VarString>(args, 0, fn);
    valt_or_fail<VarString>(args, 1, fn);
    auto s = get<VarString>(*args[0]).s;
    auto t = get<VarString>(*args[1]).s;
    return make_shared<Var>(VarBool{f(s, t)});
}

bool spred_eqp(const string & s, const string & t) { return s == t; }
EnvEntry f_stringeqp(span<EnvEntry> args)
{
    return spred(args, "string=?", spred_eqp);
}

bool spred_ltp(const string & s, const string & t) { return s < t; }
EnvEntry f_stringltp(span<EnvEntry> args)
{
    return spred(args, "string<?", spred_ltp);
}

bool spred_gtp(const string & s, const string & t) { return s > t; }
EnvEntry f_stringgtp(span<EnvEntry> args)
{
    return spred(args, "string>?", spred_gtp);
}

EnvEntry f_string_z_number(span<EnvEntry> args)
{
    if (args.size() < 1) throw RunError("string->number argc");
    valt_or_fail<VarString>(args, 0, "string->number");
    auto s = get<VarString>(*args[0]).s;
    int radix = 10;
    if (args.size() >= 2) {
        radix = get<VarNum>(*args[2]).i;
        if (radix < 2 or radix > 36)
            throw RunError("string->number radix");
    }
    auto r = strtoll(s.data(), nullptr, radix);
    return make_shared<Var>(VarNum{r});
}

EnvEntry f_number_z_string(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("number->string argc");
    valt_or_fail<VarNum>(args, 0, "number->string");
    auto n = get<VarNum>(*args[0]).i;
    ostringstream oss;
    if (args.size() >= 2) {
        int radix = get<VarNum>(*args[2]).i;
        if (radix < 2 or radix > 36)
            throw RunError("number->string radix");
        oss << setbase(radix);
    }
    oss << n;
    return make_shared<Var>(VarString{oss.str()});
}

//
// type checks
//

EnvEntry f_booleanp(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("boolean? argc");
    return make_shared<Var>(VarBool{valt_in<VarBool>(*args[0])});
}
EnvEntry f_numberp(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("number? argc");
    return make_shared<Var>(VarBool{valt_in<VarNum>(*args[0])});
}

EnvEntry f_procedurep(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("procedure? argc");
    return make_shared<Var>(VarBool{
            valt_in<VarFunHost, VarFunOps>(*args[0])});
}

EnvEntry f_symbolp(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("symbol? argc");
    return make_shared<Var>(VarBool{valt_in<VarNam>(*args[0])});
}

EnvEntry f_nullp(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("null? argc");
    auto & a = *args[0];
    if (valt_in<VarList>(a) and get<VarList>(a).v.size() == 0)
        throw CoreError("empty cont-list");
    return make_shared<Var>(VarBool{
            valt_in<VarCons>(a) and nullptr == get<VarCons>(a).c});
}

EnvEntry f_listp(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("list? argc");
    auto & a = *args[0];
    if (valt_in<VarList>(a)) {
        if (get<VarList>(a).v.size() == 0)
            throw CoreError("empty cont-list");
        return make_shared<Var>(VarBool{true});
    }
    if (not valt_in<VarCons>(a))
        return make_shared<Var>(VarBool{false});

    ConsNext x = get<VarCons>(a).c;
    // warning("cons-iter")
    for (;;) {
        if (not holds_alternative<ConsPtr>(x)) break;
        if (nullptr == get<ConsPtr>(x)) break;
        x = get<ConsPtr>(x)->d;
    }
    return make_shared<Var>(VarBool{
            holds_alternative<ConsPtr>(x)});
}

EnvEntry f_pairp(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("pair? argc");
    auto & a = *args[0];
    if (valt_in<VarNonlist>(a))
        return make_shared<Var>(VarBool{true});
    if (valt_in<VarList>(a)) {
        if (get<VarList>(a).v.size() == 0)
            throw CoreError("empty cont-list");
        return make_shared<Var>(VarBool{true});
    }
    return make_shared<Var>(VarBool{
        valt_in<VarCons>(a) and nullptr != get<VarCons>(a).c});
}

EnvEntry f_contpp(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("cont?? argc");
    return make_shared<Var>(
            VarBool{valt_in<VarList, VarNonlist>(*args[0])});
}

EnvEntry f_voidp(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("void? argc");
    return make_shared<Var>(VarBool{valt_in<VarVoid>(*args[0])});
}

//
// boolean functions (one, others are macros on cond)
//

// display functions (see I/O functions)
//
//   for development purposes and as of r7rs, not suggested for
//   program utilization

//
// list-based functions
//

EnvEntry f_length(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("length argc");
    valt_or_fail<VarCons, VarList>(args, 0, "length");
    auto & a = *args[0];
    long long r{};
    if (holds_alternative<VarCons>(a)) {
        if (get<VarCons>(a).c)
            r = get<VarCons>(a).c->length();
    } else {
        r = get<VarList>(a).v.size();
    }
    return make_shared<Var>(VarNum{r});
}

EnvEntry f_apply(span<EnvEntry> args)
{
    if (args.size() != 2) throw RunError("apply argc");
    valt_or_fail<VarCons, VarList>(args, 1, "apply");
    vector<EnvEntry> e{args[0]};
    for (auto & v : normal_list(*args[1]).v)
        e.push_back(v);
    return xapply(e);
}

EnvEntry f_map(span<EnvEntry> args)
{
    if (args.size() < 1) throw RunError("map argc");
    valt_or_fail<VarFunOps, VarFunHost>(args, 0, "map");
    vector<unique_ptr<ConsOrListIter>> inputs;
    for (size_t i = 1; i != args.size(); ++i) {
        valt_or_fail<VarCons, VarList>(args, i, "map");
        inputs.push_back(make_iter(*args[i]));
    }
    vector<EnvEntry> r;
    for (;;) {
        vector<EnvEntry> w{args[0]};
        for (auto & j : inputs) {
            auto x = j->get();
            if (x) w.push_back(x);
            else goto e;
        }
        r.push_back(fun_call(w));
    }
e:
    return make_shared<Var>(VarList{move(r)});
}

//
// input/output
//

//
// variable (de)serialization
//

// TODO: read
// TODO: write

EnvEntry f_display(span<EnvEntry> args)
{
    for (auto & a : args) {
        if (&a != &args[0]) cout << ' ';
        if (holds_alternative<VarString>(*a))
            cout << get<VarString>(*a).s;
        else
            print(a, *u_names, cout);
    }
    return make_shared<Var>(VarVoid{});
}

EnvEntry f_error(span<EnvEntry> args)
{
    int i{};
    cerr << "error invoked,";
    for (auto & a : args) {
        cerr << "\nerror-args[" << i++ << "]: ";
        print(a, *u_names, cerr);
    }
    cerr << "\nsorry'bout that" << endl;
    exit(4);
}

EnvEntry f_exit(span<EnvEntry> args)
{
    valt_or_fail<VarNum>(args, 0, "exit");
    exit(get<VarNum>(*args[0]).i);
}

//
// prng
//

//
// time
//

} // ans

namespace humble {

void init_env(Names & n)
{
    u_names = &n;
    n = Names{            // note:  known names ordered
            "=>",         //        as in tok.hpp
            "else",
            "quote",
            "quasiquote",
            "unquote",
            "macro",
            "car",
            "eqv?",
            "list",
            "nonlist",
            "set!!",
            "dup",
            "error",
            "splice",
            "import",
    };

    auto & g = GlobalEnv::instance();
    g.set(n.intern("list"), make_shared<Var>(VarFunHost{ f_list }));
    g.set(n.intern("nonlist"), make_shared<Var>(VarFunHost{ f_nonlist }));
    g.set(n.intern("list-copy"), make_shared<Var>(VarFunHost{ f_list_copy }));
    g.set(n.intern("cons"), make_shared<Var>(VarFunHost{ f_cons }));
    g.set(n.intern("car"), make_shared<Var>(VarFunHost{ f_car }));
    g.set(n.intern("list-ref"), make_shared<Var>(VarFunHost{ f_list_ref }));
    g.set(n.intern("cdr"), make_shared<Var>(VarFunHost{ f_cdr }));
    g.set(n.intern("append"), make_shared<Var>(VarFunHost{ f_append }));
    g.set(n.intern("set-car!"), make_shared<Var>(VarFunHost{ f_set_carj }));
    g.set(n.intern("set-cdr!"), make_shared<Var>(VarFunHost{ f_set_cdrj }));
    g.set(n.intern("list-tail"), make_shared<Var>(VarFunHost{ f_list_tail }));
    g.set(n.intern("list-set!"), make_shared<Var>(VarFunHost{ f_list_setj }));
    g.set(n.intern("make-list"), make_shared<Var>(VarFunHost{ f_make_list }));
    g.set(n.intern("reverse"), make_shared<Var>(VarFunHost{ f_reverse }));
    g.set(n.intern("take"), make_shared<Var>(VarFunHost{ f_take }));
    g.set(n.intern("splice"), make_shared<Var>(VarFunHost{ f_splice }));
    g.set(n.intern("not"), make_shared<Var>(VarFunHost{ f_not }));
    g.set(n.intern("+"), make_shared<Var>(VarFunHost{ f_pluss }));
    g.set(n.intern("-"), make_shared<Var>(VarFunHost{ f_minus }));
    g.set(n.intern("*"), make_shared<Var>(VarFunHost{ f_multiply }));
    g.set(n.intern("/"), make_shared<Var>(VarFunHost{ f_divide }));
    g.set(n.intern("div"), make_shared<Var>(VarFunHost{ f_div }));
    g.set(n.intern("max"), make_shared<Var>(VarFunHost{ f_max }));
    g.set(n.intern("min"), make_shared<Var>(VarFunHost{ f_min }));
    g.set(n.intern("abs"), make_shared<Var>(VarFunHost{ f_abs }));
    g.set(n.intern("zero?"), make_shared<Var>(VarFunHost{ f_zerop }));
    g.set(n.intern("positive?"), make_shared<Var>(VarFunHost{ f_positivep }));
    g.set(n.intern("negative?"), make_shared<Var>(VarFunHost{ f_negativep }));
    g.set(n.intern("even?"), make_shared<Var>(VarFunHost{ f_evenp }));
    g.set(n.intern("odd?"), make_shared<Var>(VarFunHost{ f_oddp }));
    g.set(n.intern("="), make_shared<Var>(VarFunHost{ f_eq }));
    g.set(n.intern("<"), make_shared<Var>(VarFunHost{ f_lt }));
    g.set(n.intern(">"), make_shared<Var>(VarFunHost{ f_gt }));
    g.set(n.intern("<="), make_shared<Var>(VarFunHost{ f_lte }));
    g.set(n.intern(">="), make_shared<Var>(VarFunHost{ f_gte }));
    g.set(n.intern("set!"), make_shared<Var>(VarFunHost{ f_setj }));
    g.set(n.intern("set!!"), make_shared<Var>(VarFunHost{ f_setjj }));
    g.set(n.intern("alias?"), make_shared<Var>(VarFunHost{ f_aliasp }));
    g.set(n.intern("eq?"), make_shared<Var>(VarFunHost{ f_eqp }));
    g.set(n.intern("eqv?"), make_shared<Var>(VarFunHost{ f_eqp }));
    g.set(n.intern("equal?"), make_shared<Var>(VarFunHost{ f_equalp }));
    g.set(n.intern("string-ref"), make_shared<Var>(VarFunHost{ f_string_ref }));
    g.set(n.intern("string->list"), make_shared<Var>(VarFunHost{ f_string_z_list }));
    g.set(n.intern("list->string"), make_shared<Var>(VarFunHost{ f_list_z_string }));
    g.set(n.intern("symbol->string"), make_shared<Var>(VarFunHost{ f_symbol_z_string }));
    g.set(n.intern("substring"), make_shared<Var>(VarFunHost{ f_substring }));
    g.set(n.intern("substring-index"), make_shared<Var>(VarFunHost{  f_substring_index }));
    g.set(n.intern("string-length"), make_shared<Var>(VarFunHost{ f_string_length }));
    g.set(n.intern("string-append"), make_shared<Var>(VarFunHost{ f_string_append }));
    g.set(n.intern("string=?"), make_shared<Var>(VarFunHost{ f_stringeqp }));
    g.set(n.intern("string<?"), make_shared<Var>(VarFunHost{ f_stringltp }));
    g.set(n.intern("string>?"), make_shared<Var>(VarFunHost{ f_stringgtp }));
    g.set(n.intern("string->number"), make_shared<Var>(VarFunHost{ f_string_z_number }));
    g.set(n.intern("number->string"), make_shared<Var>(VarFunHost{ f_number_z_string }));
    g.set(n.intern("boolean?"), make_shared<Var>(VarFunHost{ f_booleanp }));
    g.set(n.intern("number?"), make_shared<Var>(VarFunHost{ f_numberp }));
    g.set(n.intern("procedure?"), make_shared<Var>(VarFunHost{ f_procedurep }));
    g.set(n.intern("symbol?"), make_shared<Var>(VarFunHost{ f_symbolp }));
    g.set(n.intern("null?"), make_shared<Var>(VarFunHost{ f_nullp }));
    g.set(n.intern("list?"), make_shared<Var>(VarFunHost{ f_listp }));
    g.set(n.intern("pair?"), make_shared<Var>(VarFunHost{ f_pairp }));
    g.set(n.intern("cont??"), make_shared<Var>(VarFunHost{ f_contpp }));
    g.set(n.intern("void?"), make_shared<Var>(VarFunHost{ f_voidp }));
    g.set(n.intern("display"), make_shared<Var>(VarFunHost{ f_display }));
    g.set(n.intern("length"), make_shared<Var>(VarFunHost{ f_length }));
    g.set(n.intern("apply"), make_shared<Var>(VarFunHost{ f_apply }));
    g.set(n.intern("map"), make_shared<Var>(VarFunHost{ f_map }));
    g.set(n.intern("error"), make_shared<Var>(VarFunHost{ f_error }));
    g.set(n.intern("exit"), make_shared<Var>(VarFunHost{ f_exit }));
}

} // ns

