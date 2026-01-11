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
    FunEnv activation(FunEnv & captured, bool dot, std::span<EnvEntry> args);
};

std::set<int> unbound(std::span<Lex> t, std::set<int> & defs, bool is_block);
void report_unbound(std::set<int> u, LexForm & t, Names & names);
void zloc_scopes(std::span<Lex> t, LexEnv * local_env);
LexForm compx(const std::string & s, Names & names, Macros & macros, std::set<int> env_keys);
void compx_dispose();

Lex to_lex(EnvEntry a);
EnvEntry from_lex(Lex & x);

void print(EnvEntry a, Names & n, std::ostream & os);

extern bool warn_off;
void warn(const std::string & m, std::span<EnvEntry> a);
void warn(const std::string & m, std::span<Lex> x);

} // ns

#endif
