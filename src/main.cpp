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
        print(r, names, os);
    }
}

int main()
{
    cout << "WELCOME TO HUMBLE SCHEME.  please enter an expression and then\n"
        "use a ';' character at EOL to evaluate or EOF indication to exit\n";
    Opener opener;
    auto [names, env, macros] = init_top(&opener);
    std::string line, buf;
    while (std::getline(cin, line)) {
        if (line.back() == ';') {
            auto expr = buf + line.substr(0, line.size() - 1);
            auto ast = compx(expr, names, macros, env.keys());
            run_top(ast, env, names, cout);
            cout << '\n';
        } else {
            buf += line;
        }
    }
    cout << "farewell\n";
}

