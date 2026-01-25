#include "io_functions.hpp"
#include "except.hpp"
#include "vars.hpp"
#include "cons.hpp"
#include "compx.hpp"
#include "debug.hpp"
#include <functional>
#include <sstream>
#include <fstream>

using namespace humble;
using namespace std;

namespace {

struct InputString {
    string s;
    size_t i;
    InputString(string s) : s(s), i() { }
    int get()
    {
        if (i == s.size()) return -1;
        return static_cast<unsigned char>(s[i++]);
    }
};

void delete_input_string(void * u)
{
    delete static_cast<InputString *>(u);
}

Names * u_names;

VarExt & vext_or_fail(const vector<int> & ts, span<EnvEntry> args, size_t i, string s)
{
    ostringstream oss;
    oss << s << " args[" << i << "] ";
    if (not holds_alternative<VarExt>(*args[i])) {
        oss << var_type_name(*args[i]);
        throw RunError(oss.str());
    } else if (auto u = get<VarExt>(*args[i]).t;
            find(ts.begin(), ts.end(), u) == ts.end()) {
        oss << "ext:" << u_names->get(u);
        throw RunError(oss.str());
    }
    return get<VarExt>(*args[i]);
}

int t_eof_object;
int t_input_string;

EnvEntry make_eof()
{
    return make_shared<Var>(VarExt{t_eof_object});
}

EnvEntry f_eof_objectp(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("eof-object? argc");
    return make_shared<Var>(VarBool{
            holds_alternative<VarExt>(*args[0])
            and get<VarExt>(*args[0]).t == t_eof_object});
}

EnvEntry f_portp(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("port? argc");
    if (not holds_alternative<VarExt>(*args[0]))
        return make_shared<Var>(VarBool{false});
    auto t = get<VarExt>(*args[0]).t;
    return make_shared<Var>(VarBool{t == t_eof_object
            or t == t_input_string});
}

string get_line(int k, function<int()> get)
{
    string r;
    for (;;) {
        r.push_back(k);
        k = get();
        if (k < 0 or k == 10) break;
    }
    return r;
}

EnvEntry f_open_input_string(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("open-input-string argc");
    if (not holds_alternative<VarString>(*args[0]))
        throw RunError("open-input-string args[0] takes string");
    auto r = VarExt{t_input_string};
    r.u = new InputString{get<VarString>(*args[0]).s};
    r.f = delete_input_string;
    return make_shared<Var>(move(r));
}

EnvEntry f_open_input_string_bytes(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("open-input-string-bytes argc");
    if (not holds_alternative<VarCons>(*args[0])
            and not holds_alternative<VarList>(*args[0]))
        throw RunError("open-input-string-bytes args[0] takes list");
    auto j = make_iter(*args[0]);
    string s;
    for (;;) {
        auto x = j->get();
        if (not x) break;
        if (not holds_alternative<VarNum>(*x))
            throw RunError("o-i-s-b not number");
        auto i = get<VarNum>(*x).i;
        if (i > 255)
            throw RunError("o-i-s-b big number");
        s.push_back(i);
    }
    auto r = VarExt{t_input_string};
    r.u = new InputString{s};
    r.f = delete_input_string;
    return make_shared<Var>(move(r));
}

EnvEntry f_read_byte(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("read-byte argc");
    auto & e = vext_or_fail({t_input_string}, args, 0, "read-byte");
    auto p = static_cast<InputString *>(e.u);
    auto k = p->get();
    if (k < 0) return make_eof();
    return make_shared<Var>(VarNum{k});
}

EnvEntry f_read_line(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("read-line argc");
    auto & e = vext_or_fail({t_input_string}, args, 0, "read-line");
    auto p = static_cast<InputString *>(e.u);
    auto k = p->get();
    if (k < 0) return make_eof();
    auto r = get_line(k, [&p](){ return p->get(); });
    return make_shared<Var>(VarString{r});
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
            { "port?", f_portp },
            { "eof-object?", f_eof_objectp },
            { "open-input-string", f_open_input_string },
            { "open-input-string-bytes", f_open_input_string_bytes },
            { "read-byte", f_read_byte },
            { "read-line", f_read_line },
    }) g.set(n.intern(p.first), make_shared<Var>(VarFunHost{ p.second }));
}

} // ns

