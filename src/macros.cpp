#include "macros.hpp"

#include <iostream>

using namespace std;

namespace humble {

Macros init_macros(set<int> env_keys, Names & names)
{
    auto m = qt_macros();
    cout << "hello world\n";
    (void)env_keys;
    (void)names;
    return m;
}

} // ns

