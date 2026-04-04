#include "compx.hpp"
#include "xeval.hpp"
#include "top.hpp"
#include "functions.hpp"
#include "io_functions.hpp"
#include "except.hpp"
#include <fstream>
#include <iostream>
#include <list>
#include <cstring>

using namespace humble;
using namespace std;

GlobalEnv init_top(Macros & macros)
{
    macros_init(macros);
    auto & g = GlobalEnv::initial();
    return g.init();
}

void errout(const string & ty, const string & wh, const string & fn)
{
    cerr << ty;
    if (not fn.empty())
        cerr << " in " << fn;
    cerr << ",\n" << wh << endl;
}

void run_top(LexForm & ast, string src, Names & names, Macros & macros,
        GlobalEnv & env, string & fn, vector<LexEnv *> & local_envs, LibLoader & loader)
    try
{
    auto t = parse(src, names, macros);
    loader(env, names, cerr);
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

