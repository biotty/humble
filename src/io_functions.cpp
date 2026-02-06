#include "io_functions.hpp"
#include "except.hpp"
#include "vars.hpp"
#include "cons.hpp"
#include "compx.hpp"
#include "xeval.hpp"
#include "dl.hpp"
#include "fun_impl.hpp"
#include "debug.hpp"
#include <functional>
#include <sstream>
#include <fstream>

// for pipe
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// for misc
#include <ctime>
#include <cmath>

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

constexpr int MAX_ARGV = 30;

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
                throw RunError("input-command element not string");
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

struct OutputString {
    string s;
    OutputString() { }
    void put(int i)
    {
        s.push_back(static_cast<unsigned char>(i));
    }
    void put(string t)
    {
        s += t;
    }
};

void delete_output_string(void * u)
{
    delete static_cast<OutputString *>(u);
}

struct OutputFile {
    ofstream ofs;
    OutputFile(string s) : ofs(s, std::ios_base::binary)
    {
        if (not ofs.is_open()) throw std::runtime_error(
                "Failed to open output-file by name '" + s + "'");
    }
    void put(int i)
    {
        unsigned char r = i;
        if (ofs.write(reinterpret_cast<char *>(&r), 1)) return;
        if (not ofs.eof()) warn("file-write error");
        ofs.close();
    }
    void put(string s)
    {
        ofs << s;
        if (not ofs) {
            warn("file-write error");
            ofs.close();
        }
    }
};

void delete_output_file(void * u)
{
    delete static_cast<OutputFile *>(u);
}

struct OutputPipe {
    int fd;
    int pid;
    OutputPipe(unique_ptr<ConsOrListIter> j)
    {
        char * argv[MAX_ARGV + 1] = {};
        for (int i = 0; i < MAX_ARGV; ++i) {
            auto x = j->get();
            if (not x) break;
            if (not holds_alternative<VarString>(*x))
                throw RunError("output-command element not string");
            auto s = ::get<VarString>(*x).s;
            argv[i] = strdup(s.c_str());
        }
        int pfd[2];
        if (pipe(pfd) == -1) {
            perror("pipe");
            exit(1);
        }
        fd = pfd[1];
        pid = fork();
        if (pid == -1) {
            perror("fork");
        } else if (pid == 0) {
            close(fd);
            dup2(pfd[0], 0);
            execvp(argv[0], argv);
            _Exit(1);
        } else {
            close(pfd[0]);
        }
        for (int i = 0; argv[i]; ++i) free(argv[i]);
    }
    bool complete_write(const char * s, size_t n)
    {
        size_t i{};
        while (i != n) {
            int r = write(fd, s, n);
            if (r < 0) {
                if (errno == EAGAIN) continue;
                if (errno == EINTR) continue;
                return false;
            }
            s += r;
            n -= r;
        }
        return true;
    }
    void put(int i)
    {
        if (fd < 0) return;
        unsigned char c = i;
        if (complete_write(reinterpret_cast<char *>(&c), 1))
            return;
        perror("write");
        close(fd);
        fd = -1;
    }
    void put(string s)
    {
        if (fd < 0) return;
        if (complete_write(s.c_str(), s.size()))
            return;
        perror("write");
        close(fd);
        fd = -1;
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
    ~OutputPipe()
    {
        int i;
        if (fd >= 0) close(fd);
        if (pid > 0) waitpid(pid, &i, 0);
    }
};

void delete_output_pipe(void * u)
{
    delete static_cast<OutputPipe *>(u);
}

struct PrngState {
    char statebuf[32];
    random_data buf;
    PrngState(unsigned int seed) {
        buf.state = NULL;
        initstate_r(0, statebuf, sizeof statebuf, &buf);
        srandom_r(seed, &buf);
    }
};

void delete_prng_state(void * u)
{
    delete static_cast<PrngState *>(u);
}

int t_eof_object;
int t_input_string;
int t_input_file;
int t_input_pipe;
int t_output_string;
int t_output_file;
int t_output_pipe;
int t_prng_state;

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
            or t == t_input_pipe
            or t == t_output_string
            or t == t_output_file
            or t == t_output_pipe});
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
    valt_or_fail<VarString>(args, 0, "open-input-string");
    auto r = VarExt{t_input_string};
    r.u = new InputString{get<VarString>(*args[0]).s};
    r.f = delete_input_string;
    return make_shared<Var>(move(r));
}

EnvEntry f_open_input_string_bytes(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("o-i-s-b argc");
    valt_or_fail<VarCons, VarList>(args, 0, "o-i-s-b");
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
    valt_or_fail<VarString>(args, 0, "open-input-file");
    auto r = VarExt{t_input_file};
    r.u = new InputFile{get<VarString>(*args[0]).s};
    r.f = delete_input_file;
    return make_shared<Var>(move(r));
}

EnvEntry f_with_input_pipe(span<EnvEntry> args)
{
    if (args.size() != 2) throw RunError("with-input-pipe argc");
    valt_or_fail<VarCons, VarList>(args, 0, "with-input-pipe");
    valt_or_fail<VarFunHost, VarFunOps>(args, 1, "with-input-pipe");
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

EnvEntry f_open_output_string(span<EnvEntry> args)
{
    if (args.size() != 0) throw RunError("open-output-string argc");
    auto r = VarExt{t_output_string};
    r.u = new OutputString{};
    r.f = delete_output_string;
    return make_shared<Var>(move(r));
}

EnvEntry f_output_string_get(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("output-string-get argc");
    auto & e = vext_or_fail({t_output_string}, args, 0, "output-string-get");
    auto p = static_cast<OutputString *>(e.u);
    return make_shared<Var>(VarString{p->s});
}

EnvEntry f_output_string_get_bytes(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("o-s-g-b argc");
    auto & e = vext_or_fail({t_output_string}, args, 0, "o-s-g-b");
    auto p = static_cast<OutputString *>(e.u);
    vector<EnvEntry> r;
    for (auto i : p->s) {
        int u = static_cast<unsigned char>(i);
        r.push_back(make_shared<Var>(VarNum{u}));
    }
    return make_shared<Var>(VarList{move(r)});
}

EnvEntry f_open_output_file(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("open-output-file argc");
    valt_or_fail<VarString>(args, 0, "open-output-file");
    auto r = VarExt{t_output_file};
    r.u = new OutputFile{get<VarString>(*args[0]).s};
    r.f = delete_output_file;
    return make_shared<Var>(move(r));
}

EnvEntry f_with_output_pipe(span<EnvEntry> args)
{
    if (args.size() != 2) throw RunError("with-output-pipe argc");
    valt_or_fail<VarCons, VarList>(args, 0, "with-output-pipe");
    valt_or_fail<VarFunHost, VarFunOps>(args, 1, "with-output-pipe");
    auto p = new OutputPipe{make_iter(*args[0])};
    auto k = make_shared<Var>(VarExt{t_output_pipe});
    get<VarExt>(*k).u = p;
    get<VarExt>(*k).f = delete_output_pipe;
    vector<EnvEntry> x{args[1], k};
    auto y = fun_call(x);
    int status = p->done();
    vector<EnvEntry> v{make_shared<Var>(VarNum{status}), y};
    return make_shared<Var>(VarNonlist{move(v)});
}

EnvEntry f_write_byte(span<EnvEntry> args)
{
    if (args.size() != 2) throw RunError("write-byte argc");
    valt_or_fail<VarNum>(args, 0, "write-byte");
    auto & e = vext_or_fail(
            {t_output_string, t_output_file, t_output_pipe},
            args, 1, "write-byte");
    int i = get<VarNum>(*args[0]).i;
    if (e.t == t_output_string)
        static_cast<OutputString *>(e.u)->put(i);
    else if (e.t == t_output_file)
        static_cast<OutputFile *>(e.u)->put(i);
    else if (e.t == t_output_pipe)
        static_cast<OutputPipe *>(e.u)->put(i);
    else abort();
    return make_shared<Var>(VarVoid{});
}

EnvEntry f_write_string(span<EnvEntry> args)
{
    if (args.size() != 2) throw RunError("write-string argc");
    valt_or_fail<VarString>(args, 0, "write-string");
    auto & e = vext_or_fail(
            {t_output_string, t_output_file, t_output_pipe},
            args, 1, "write-string");
    string s = get<VarString>(*args[0]).s;
    if (e.t == t_output_string)
        static_cast<OutputString *>(e.u)->put(s);
    else if (e.t == t_output_file)
        static_cast<OutputFile *>(e.u)->put(s);
    else if (e.t == t_output_pipe)
        static_cast<OutputPipe *>(e.u)->put(s);
    else abort();
    return make_shared<Var>(VarVoid{});
}

EnvEntry f_clock(span<EnvEntry> args)
{
    if (args.size() != 0) throw RunError("clock argc");
    time_t r;
    (void)time(&r);
    return make_shared<Var>(VarNum{r});
}

constexpr long long JIFFIES_PER_SECOND = 1000;
static timespec u_zt;

EnvEntry f_current_jiffy(span<EnvEntry> args)
{
    if (args.size() != 0) throw RunError("current-jiffy argc");
    long long r{};
    if (u_zt.tv_sec == 0) {
        (void)clock_gettime(CLOCK_MONOTONIC, &u_zt);
    } else {
        timespec ts{};
        (void)clock_gettime(CLOCK_MONOTONIC, &ts);
        r = ((ts.tv_sec - u_zt.tv_sec)
                + 1e-9 * double(ts.tv_nsec - u_zt.tv_nsec))
            * JIFFIES_PER_SECOND;
    }
    return make_shared<Var>(VarNum{r});
}

EnvEntry f_pause(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("pause argc");
    valt_or_fail<VarNum>(args, 0, "pause");
    auto p = get<VarNum>(*args[0]).i * 1e9 / JIFFIES_PER_SECOND;
    if (p > 0) {
        timespec ts;
        timespec rem;
        ts.tv_sec = p * 1e-9;
        ts.tv_nsec = fmod(p, 1e9);
        nanosleep(&ts, &rem);
        // TODO: on ind re-do w rem
    }
    return make_shared<Var>(VarVoid{});
}

EnvEntry f_make_prng_state(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("make-prng-state argc");
    valt_or_fail<VarNum>(args, 0, "make-prng-state");
    unsigned int seed = get<VarNum>(*args[0]).i;
    auto r = VarExt{t_prng_state};
    r.u = new PrngState(seed);
    r.f = delete_prng_state;
    return make_shared<Var>(move(r));
}

EnvEntry f_prng_get(span<EnvEntry> args)
{
    if (args.size() != 1) throw RunError("prng-get argc");
    auto & e = vext_or_fail({t_prng_state}, args, 0, "prng-get");
    int32_t result;
    (void)random_r(&static_cast<PrngState *>(e.u)->buf, &result);
    return make_shared<Var>(VarNum{result});
}

vector<string> u_command_line;

EnvEntry f_command_line(span<EnvEntry> args)
{
    if (args.size() != 0) throw RunError("command-line argc");
    vector<EnvEntry> result;
    for (auto & s : u_command_line) {
        result.push_back(make_shared<Var>(VarString{s}));
    }
    return make_shared<Var>(VarList{result});
}

} // ans

namespace humble {

void io_set_command_line(int argc, char ** argv)
{
    for (int i = 1; i != argc; ++i) u_command_line.push_back(argv[i]);
    // skip humble itself
}

void io_functions(Names & n)
{
    if (u_names and u_names != &n)
        throw CoreError("io_functions on separate intern");
    u_names = &n;
    t_eof_object = n.intern("eof-object");
    t_input_string = n.intern("input-string");
    t_input_file = n.intern("input-file");
    t_input_pipe = n.intern("input-pipe");
    t_output_string = n.intern("output-string");
    t_output_file = n.intern("output-file");
    t_output_pipe = n.intern("output-pipe");
    t_prng_state = n.intern("prng-state");
    auto & g = GlobalEnv::initial();
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
            { "open-output-string", f_open_output_string },
            { "open-output-file", f_open_output_file },
            { "with-output-pipe", f_with_output_pipe },
            { "output-string-get", f_output_string_get },
            { "output-string-get-bytes", f_output_string_get_bytes },
            { "write-byte", f_write_byte },
            { "write-string", f_write_string },
            { "clock", f_clock },
            { "current-jiffy", f_current_jiffy },
            { "pause", f_pause },
            { "make-prng-state", f_make_prng_state },
            { "prng-get", f_prng_get },
            { "command-line", f_command_line },
    }) g.set(n.intern(p.first), make_shared<Var>(VarFunHost{ p.second }));
}

} // ns

