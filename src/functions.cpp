#include "functions.hpp"
#include "except.hpp"
#include "vars.hpp"
#include "cons.hpp"
#include "compx.hpp"
#include "debug.hpp"

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
                throw CoreError("short nonlist");
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
    Cons * q{};
    for (size_t i = 1; i != args.size(); ++i) {
        if (holds_alternative<ConsPtr>(p)
                and get<ConsPtr>(p) != nullptr)
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
    return make_shared<Var>(VarCons{r});
}

EnvEntry f_splice(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("splice argc");
    valt_or_fail<VarCons, VarList>(args, 0, "splice");
    return make_shared<Var>(VarSplice{normal_list(*args[0]).v});
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

//
// mutation
//

EnvEntry setjj(span<EnvEntry> args)
{
    if (&*args[0] == &*args[1]) {
#ifdef DEBUG
        cout << "self-set\n";
#endif
    } else {
        visit([&args](auto && w) { *args[0] = w; }, *args[1]);
    }
    return make_shared<Var>(VarVoid{});
}

EnvEntry f_setj(span<EnvEntry> args)
{
    if (args.size() != 2) throw RunError("set! argc");
    if (args[0]->index() != args[1]->index()) {
#ifdef DEBUG
        cout << "set! to different type\n";
#endif
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
    } else {
        r = (&a == &b);
    }
    return make_shared<Var>(VarBool{r});
}

//
// DICT
//

//
// REC
//

//
// string
//

//
// type checks
//

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
// list usage
//

//
// input/output
//

//
// prng
//

//
// time
//

Names * u_names;

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
    g.set(n.intern("splice"), make_shared<Var>(VarFunHost{ f_splice }));
    g.set(n.intern("+"), make_shared<Var>(VarFunHost{ f_pluss }));
    g.set(n.intern("set!"), make_shared<Var>(VarFunHost{ f_setj }));
    g.set(n.intern("set!!"), make_shared<Var>(VarFunHost{ f_setjj }));
    g.set(n.intern("alias?"), make_shared<Var>(VarFunHost{ f_aliasp }));
    g.set(n.intern("eq?"), make_shared<Var>(VarFunHost{ f_eqp }));
    g.set(n.intern("cont??"), make_shared<Var>(VarFunHost{ f_contpp }));
    g.set(n.intern("void?"), make_shared<Var>(VarFunHost{ f_voidp }));
    g.set(n.intern("display"), make_shared<Var>(VarFunHost{ f_display }));
}

void print(EnvEntry a, Names & n, std::ostream & os)
{
    print(to_lex(a), n, os);
}

} // ns

