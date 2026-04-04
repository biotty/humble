#include "compx.hpp"
#include "xeval.hpp"
#include "top.hpp"
#include "functions.hpp"
#include "io_functions.hpp"
#include "dl.hpp"
#include "except.hpp"
#include <fstream>
#include <iostream>
#include <list>
#include <cstring>

// dlopen
#include <dlfcn.h>

using namespace humble;
using namespace std;

struct Opener : SrcOpener {
    constexpr static struct noresolve_t { } noresolve { };

    string src_dir;
    Opener(string src_dir) : src_dir(src_dir) { }

    string operator()(string name) override
    {
        if (not name.contains('/'))
            name = src_dir + "/" + name;
        return operator()(name, noresolve);
    }

    string operator()(string name, noresolve_t)
    {
        filename = name;
        ifstream f(name, std::ios_base::binary);
        if (not f.is_open()) {
            throw std::runtime_error("Failed to open source-file by name"
                    " '" + name + "'");
        }
        return string{(istreambuf_iterator<char>(f)), 
            istreambuf_iterator<char>()};
    }
};

void errout(const string & ty, const string & wh, const string & fn)
{
    cerr << ty;
    if (not fn.empty())
        cerr << " in " << fn;
    cerr << ",\n" << wh << endl;
}

struct LibLoader {
    string libs_dir;
    set<string> requires_accum;
    LibLoader(string libs_dir) : libs_dir(libs_dir) { }

    unique_ptr<Macro> requires_macro()
    {
        struct Requires : MacroClone<Requires> {
            set<string> * accum;
            Requires(set<string> & accum) : accum(&accum) { }
            Lex operator()(LexForm && s) override
            {
                if (s.v.size() != 2) throw SrcError("requires argc");
                if (not holds_alternative<LexString>(s.v[1])) throw SrcError("requires");
                accum->insert(get<LexString>(s.v[1]).s);
                return LexVoid{};
            }
        };
        return make_unique<Requires>(requires_accum);
    }

    void load_lib(string name, GlobalEnv & env, Names & n)
    {
        dl_arg u_arg = { &n, &env };
        string path = libs_dir + "/libH" + name + ".so";
        string sym = "dl_" + name;
        // consider: ^ use mere symbol "init" always, if allows
        void * dl = dlopen(path.c_str(), RTLD_NOW);
        if (not dl) {
            cerr << path << " not loaded: " << dlerror() << endl;
            exit(1);
        } else {
            auto f = (dl_fn)dlsym(dl, sym.c_str());
            if (not f) {
                cerr << sym << ": " << dlerror() << endl;
                exit(1);
            } else {
                f(&u_arg);
            }
        }
    }

    void operator()(GlobalEnv & env, Names & n)
    {
        for (const string & s : requires_accum)
            load_lib(s, env, n);
    }
};

void run_top(LexForm & ast, string src, Names & names, Macros & macros,
        GlobalEnv & env, string & fn, vector<LexEnv *> & local_envs, LibLoader & loader)
    try
{
    auto t = parse(src, names, macros);
    loader(env, names);
    ast = compx(move(t), names, env.keys(), local_envs);
    auto & os = cout;
    for (auto & a : ast.v) {
        auto r = run(a, env);
        if (not holds_alternative<VarVoid>(*r)) {
            os << "; ==> ";
            print(r, names, os);
            os << endl;
        }
    }
} catch (const SrcError & e) {
    errout("src-error", e.what(), fn);
} catch (const RunError & e) {
    errout("run-error", e.what(), fn);
} catch (const runtime_error & e) {
    errout("error", e.what(), fn);
}

int main(int argc, char ** argv)
{
    string home = getenv("HOME");
    if (home.empty()) exit(1);
    string dir = home + "/.local/humble";

    vector<LexEnv *> local_envs;

    Names names = init_names();
    init_functions(names);
    io_functions(names);
    // ^ also serves as example of extension types, VarExt
    io_set_system_command_line(argc, argv);

    Macros macros;
    Opener opener{ dir };
    init_macros(macros, names, opener, local_envs);
    LibLoader loader{ dir };
    macros[names.intern("requires")] = loader.requires_macro();
    top_included(names, macros, local_envs);
    auto env = init_top(macros);

    if (argc >= 2) {
        char * fn = argv[1];
        auto src = opener(fn, Opener::noresolve);
        LexForm ast;
        run_top(ast, src, names, macros, env, opener.filename, local_envs, loader);
        compx_dispose(local_envs);
        exit(0);
    }

    cout << "WELCOME TO HUMBLE SCHEME.  please enter an expression and then\n"
        "use a ';' character at EOL to evaluate or EOF indication to exit\n";
    list<LexForm> x;
    std::string line, buf;
    while (cout << ":" << flush and std::getline(cin, line)) {
        if (size_t i = line.find_first_not_of(":"); line.npos != i) line.erase(0, i);
        if (size_t i = line.find_last_not_of(" \t"); line.npos != i) line.erase(i + 1);
        buf += line + "\n";
        if (line.back() == ';') {
            x.push_back(LexForm{});
            run_top(x.back(), buf, names, macros, env, opener.filename, local_envs, loader);
            buf.clear();
        }
    }
    cout << "\nfare well.\n";
    compx_dispose(local_envs);
}

