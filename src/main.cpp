#include "compx.hpp"
#include "xeval.hpp"
#include "top.hpp"
#include <fstream>
#include <iostream>

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
            os << "  ==> ";
            print(r, names, os);
            os << '\n';
        }
    }
}

int main(int argc, char ** argv)
{
    atexit(compx_dispose);
    Opener opener;
    auto [names, env, macros] = init_top(&opener);
    if (argc == 2) {
        auto x = opener(argv[1]);
        auto t = compx(x, names, macros, env.keys());
        run_top(t, env, names, cout);
        exit(0);
    }
    cout << "WELCOME TO HUMBLE SCHEME.  please enter an expression and then\n"
        "use a ';' character at EOL to evaluate or EOF indication to exit\n";
    LexForm x;
    std::string line, buf;
    while (std::getline(cin, line)) {
        if (line.back() == ';') {
            auto expr = buf + line.substr(0, line.size() - 1);
            LexForm ast;
            try {
                ast = compx(expr, names, macros, env.keys());
                run_top(ast, env, names, cout);
            } catch (const runtime_error & e) {
                cout << e.what();
            }
            move(ast.v.begin(), ast.v.end(), back_inserter(x.v));
        } else {
            buf += line;
        }
    }
    cout << "farewell\n";
}

