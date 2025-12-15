#include "xeval.hpp"

using namespace std;

namespace humble {

EnvEntry xeval(Lex & x, Env & env)
{
    (void)env;
    if (auto p = get_if<LexNum>(&x); p)
        return make_shared<Var>(VarNum{p->i});
    // TODO: instead do std::visit on x
    return make_shared<Var>(VarVoid{});
}

} // ns

