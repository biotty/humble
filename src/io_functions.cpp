#include "io_functions.hpp"
#include "except.hpp"
#include "vars.hpp"
#include "cons.hpp"
#include "compx.hpp"
#include "xeval.hpp"
#include "debug.hpp"
#include <functional>
#include <sstream>
#include <fstream>

// for pipe
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

constexpr int MAX_ARGV = 30;

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

struct InputFile {
    ifstream ifs;
    InputFile(string s) : ifs(s, std::ios_base::binary)
    {
        if (not ifs.is_open()) throw std::runtime_error(
                "Failed to open input-file by name '" + s + "'");
    }
    int get()
    {
        unsigned char r;
        if (ifs.read(reinterpret_cast<char *>(&r), 1)) return r;
        if (not ifs.eof()) warn("file-read error");
        ifs.close();
        return -1;
    }
};

void delete_input_file(void * u)
{
    delete static_cast<InputFile *>(u);
}

struct InputPipe {
    int fd;
    int pid;
    InputPipe(unique_ptr<ConsOrListIter> j)
    {
        char * argv[MAX_ARGV + 1] = {};
        for (int i = 0; i < MAX_ARGV; ++i) {
            auto x = j->get();
            if (not x) break;
            if (not holds_alternative<VarString>(*x))
                throw RunError("command element not string");
            auto s = ::get<VarString>(*x).s;
            argv[i] = strdup(s.c_str());
        }
        int pfd[2];
        if (pipe(pfd) == -1) {
            perror("pipe");
            exit(1);
        }
        fd = pfd[0];
        pid = fork();
        if (pid == -1) {
            perror("fork");
        } else if (pid == 0) {
            close(fd);
            int null = open("/dev/null", 0);
            dup2(null, 0);
            dup2(pfd[1], 1);
            execvp(argv[0], argv);
            _Exit(1);
        } else {
            close(pfd[1]);
        }
        for (int i = 0; argv[i]; ++i) free(argv[i]);
    }
    int get()
    {
        if (fd < 0) return -1;
        unsigned char r;
        int i = read(fd, reinterpret_cast<void *>(&r), 1);
        if (i == 1) return r;
        if (i) {
            perror("read");
            close(fd);
        }
        fd = -1;
        return -1;
    }
    [[nodiscard]] int done()
    {
        if (fd >= 0) {
            close(fd);
            fd = -1;
        }
        int status = -1;
        if (pid > 0) {
            if (waitpid(pid, &status, 0) == -1) perror("waitpid");
            pid = -1;
        }
        return status;
    }
    ~InputPipe()
    {
        int i;
        if (fd >= 0) close(fd);
        if (pid > 0) waitpid(pid, &i, 0);
    }
};

void delete_input_pipe(void * u)
{
    delete static_cast<InputPipe *>(u);
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
int t_input_file;
int t_input_pipe;

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
    return make_shared<Var>(VarBool{
            t == t_eof_object
            or t == t_input_string
            or t == t_input_file
            or t == t_input_pipe});
}

string get_line(int k, function<int()> get)
{
    string r;
    while (k >= 0 and k != 10) {
        r.push_back(k);
        k = get();
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
        s.push_back(static_cast<unsigned char>(i));
    }
    auto r = VarExt{t_input_string};
    r.u = new InputString{s};
    r.f = delete_input_string;
    return make_shared<Var>(move(r));
}

EnvEntry f_open_input_file(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("open-input-file argc");
    if (not holds_alternative<VarString>(*args[0]))
        throw RunError("open-input-file args[0] takes string");
    auto r = VarExt{t_input_file};
    r.u = new InputFile{get<VarString>(*args[0]).s};
    r.f = delete_input_file;
    return make_shared<Var>(move(r));
}

EnvEntry f_with_input_pipe(span<EnvEntry> args)
{
    if (args.size() != 2) throw RunError("with-input-pipe argc");
    if (not holds_alternative<VarList>(*args[0])
            and not holds_alternative<VarCons>(*args[0]))
        throw RunError("with-input-pipe args[0] takes list");
    if (not holds_alternative<VarFunHost>(*args[1])
            and not holds_alternative<VarFunOps>(*args[1]))
        throw RunError("with-input-pipe args[1] takes procedure");
    auto p = new InputPipe{make_iter(*args[0])};
    auto k = make_shared<Var>(VarExt{t_input_pipe});
    get<VarExt>(*k).u = p;
    get<VarExt>(*k).f = delete_input_pipe;
    vector<EnvEntry> x{args[1], k};
    auto y = fun_call(x);
    int status = p->done();
    vector<EnvEntry> v{make_shared<Var>(VarNum{status}), y};
    return make_shared<Var>(VarNonlist{move(v)});
}

EnvEntry f_read_byte(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("read-byte argc");
    auto & e = vext_or_fail(
            {t_input_string, t_input_file, t_input_pipe},
            args, 0, "read-byte");
    int k;
    if (e.t == t_input_string)
        k = static_cast<InputString *>(e.u)->get();
    else if (e.t == t_input_file)
        k = static_cast<InputFile *>(e.u)->get();
    else if (e.t == t_input_pipe)
        k = static_cast<InputPipe *>(e.u)->get();
    else abort();
    if (k < 0) return make_eof();
    return make_shared<Var>(VarNum{k});
}

EnvEntry f_read_line(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("read-line argc");
    auto & e = vext_or_fail(
            {t_input_string, t_input_file, t_input_pipe},
            args, 0, "read-line");
    string r;
    if (e.t == t_input_string) {
        auto p = static_cast<InputString *>(e.u);
        if (auto k = p->get(); k < 0) return make_eof();
        else r = get_line(k, [&p](){ return p->get(); });
    } else if (e.t == t_input_file) {
        auto p = static_cast<InputFile *>(e.u);
        if (auto k = p->get(); k < 0) return make_eof();
        else r = get_line(k, [&p](){ return p->get(); });
    } else if (e.t == t_input_pipe) {
        auto p = static_cast<InputPipe *>(e.u);
        if (auto k = p->get(); k < 0) return make_eof();
        else r = get_line(k, [&p](){ return p->get(); });
    } else abort();
    return make_shared<Var>(VarString{r});
}

} // ans

namespace humble {

void io_functions(Names & n)
{
    t_eof_object = n.intern("eof-object");
    t_input_string = n.intern("input-string");
    t_input_file = n.intern("input-file");
    t_input_pipe = n.intern("input-pipe");
    auto & g = GlobalEnv::instance();
    typedef EnvEntry (*hp)(span<EnvEntry> args);
    for (auto & p : initializer_list<pair<string, hp>>{
            { "port?", f_portp },
            { "eof-object?", f_eof_objectp },
            { "open-input-string", f_open_input_string },
            { "open-input-string-bytes", f_open_input_string_bytes },
            { "open-input-file", f_open_input_file },
            { "with-input-pipe", f_with_input_pipe },
            { "read-byte", f_read_byte },
            { "read-line", f_read_line },
    }) g.set(n.intern(p.first), make_shared<Var>(VarFunHost{ p.second }));
}

} // ns

