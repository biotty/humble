#include "io_functions.hpp"
#include "except.hpp"
#include "vars.hpp"
#include "compx.hpp"
#include "debug.hpp"
#include <sstream>
#include <fstream>

using namespace humble;
using namespace std;

namespace {

int t_eof_object;
int t_input_string;

struct InputString {
    string s;
    size_t i;
};

EnvEntry f_eof_objectp(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("eof-object? argc");
    return make_shared<Var>(VarBool{
            holds_alternative<VarExt>(*args[0])
            and get<VarExt>(*args[0]).t == t_eof_object});
}

void delete_input_string(void * u)
{
    delete static_cast<InputString *>(u);
}

EnvEntry f_open_input_string(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("open-input-string argc");
    if (not holds_alternative<VarString>(*args[0]))
        throw RunError("open-input-string args[0] takes string");

    auto r = VarExt{t_input_string};
    r.u = new InputString{get<VarString>(*args[0]).s, 0u};
    r.f = delete_input_string;
    return make_shared<Var>(move(r));
}

EnvEntry f_read_byte(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("read-byte argc");
    if (not holds_alternative<VarExt>(*args[0]))
        throw RunError("read-byte args[0] takes ext");
    auto & e = get<VarExt>(*args[0]);
    if (e.t != t_input_string)
        throw RunError("open-input-string args[0] takes ext:input-string");

    auto p = static_cast<InputString *>(e.u);
    if (p->i == p->s.size())
        return make_shared<Var>(VarExt{t_eof_object});
    unsigned char c = p->s[p->i++];
    return make_shared<Var>(VarNum{c});
}

} // ans

namespace humble {

void io_functions(Names & n)
{
    t_eof_object = n.intern("eof-object");
    t_input_string = n.intern("input-string");
    auto & g = GlobalEnv::instance();
    typedef EnvEntry (*hp)(span<EnvEntry> args);
    for (auto & p : initializer_list<pair<string, hp>>{
            { "eof-object?", f_eof_objectp },
            { "open-input-string", f_open_input_string },
            { "read-byte", f_read_byte }
    }) g.set(n.intern(p.first), make_shared<Var>(VarFunHost{ p.second }));
}

} // ns

