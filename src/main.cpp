#include "compx.hpp"
#include "xeval.hpp"
#include "top.hpp"
#include "functions.hpp"
#include "io_functions.hpp"
#include "except.hpp"
#include <fstream>
#include <iostream>
#include <list>

using namespace humble;
using namespace std;

struct Opener : SrcOpener {
    string operator()(string name) override {
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

void run_top(LexForm & ast, GlobalEnv & env, Names & names, ostream & os)
{
    for (auto & a : ast.v) {
        auto r = run(a, env);
        if (not holds_alternative<VarVoid>(*r)) {
            os << "; ==> ";
            print(r, names, os);
            os << endl;
        }
    }
}

void errout(const string & ty, const string & wh, const string & fn)
{
    cerr << ty;
    if (not fn.empty())
        cerr << " in " << fn;
    cerr << ",\n" << wh << endl;
}

void compxrun(LexForm & ast, string src, Names & names, Macros & macros, GlobalEnv & env, string & fn)
    try
{
    ast = compx(src, names, macros, env.keys());
    run_top(ast, env, names, cout);
} catch (const SrcError & e) {
    errout("src-error", e.what(), fn);
} catch (const RunError & e) {
    errout("run-error", e.what(), fn);
} catch (const runtime_error & e) {
    errout("error", e.what(), fn);
}

int main(int argc, char ** argv)
{
    atexit(compx_dispose);
    // todo: ^ mechanism such as hold by global unique_ptr
    // there instead. can still dispose explicitly in tests.
    Names names = init_names();
    init_functions(names);
    io_functions(names);
    // placeholder: reg more functions
    Macros macros;
    Opener opener;
    init_macros(macros, names, opener);
    // evt: insert here more language-macros
    top_included(names, macros);
    auto env = init_top(macros);
    if (argc == 2) {
        auto src = opener(argv[1]);
        LexForm ast;
        compxrun(ast, src, names, macros, env, opener.filename);
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
            compxrun(x.back(), buf, names, macros, env, opener.filename);
            buf.clear();
        }
    }
    cout << "\nfare well.\n";
}

