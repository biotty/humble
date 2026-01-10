#include "compx.hpp"
#include "xeval.hpp"
#include "top.hpp"
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
    try
{
    for (auto & a : ast.v) {
        auto r = run(a, env);
        if (not holds_alternative<VarVoid>(*r)) {
            os << "  ==> ";
            print(r, names, os);
            os << '\n';
        }
    }
} catch (const SrcError & e) {
    cout << "src error: " << e.what() << endl;
} catch (const RunError & e) {
    cout << "run error: " << e.what() << endl;
} catch (const runtime_error & e) {
    cout << "error: " << e.what() << endl;
}

int main(int argc, char ** argv)
{
    atexit(compx_dispose);
    Names names;
    Macros macros;
    Opener opener;
    auto env = init_top(names, macros, opener);
    if (argc == 2) {
        auto x = opener(argv[1]);
        auto t = compx(x, names, macros, env.keys());
        run_top(t, env, names, cout);
        exit(0);
    }
    cout << "WELCOME TO HUMBLE SCHEME.  please enter an expression and then\n"
        "use a ';' character at EOL to evaluate or EOF indication to exit\n";
    list<LexForm> x;
    std::string line, buf;
    while (cout << ":" << flush
            and std::getline(cin, line)) {
        if (line.back() == ';') {
            auto expr = buf + line.substr(0, line.size() - 1);
                x.push_back(LexForm{});
                auto & ast = x.back();
                ast = compx(expr, names, macros, env.keys());
                run_top(ast, env, names, cout);
        } else {
            buf += line;
        }
    }
    cout << "\nfare well.\n";
}

