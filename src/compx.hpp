#ifndef HUMBLE_COMPX
#define HUMBLE_COMPX

#include "tok.hpp"
#include "parse.hpp"
#include "vars.hpp"
#include <set>
#include <span>

namespace humble {

class LexEnv {
    size_t n_parms;
    std::vector<int> names;
    size_t n_init;
public:
    LexEnv(const std::vector<int> & parms, const std::vector<int> & capture);
    std::vector<int> parms() const;
    std::vector<int> capture() const;
    std::vector<int> rewrite_names(const std::vector<int> & c);
    int rewrite_name(int n);
    FunEnv activation(FunEnv & captured, bool dot, FunEnv & args);
};

std::set<int> unbound(std::span<Lex> t, std::set<int> & defs, bool is_block);
LexForm compx(const std::string & s, Names & names, Macros & macros, std::set<int> env_keys);
void compx_dispose();

} // ns

#endif
