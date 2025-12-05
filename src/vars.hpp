#ifndef HUMBLE_VARS
#define HUMBLE_VARS

#include <string>
#include <vector>
#include <variant>
#include <map>
#include <memory>

namespace humble {

struct VarNum { long long i; };
struct VarString { std::string s; };
// idea: have BigStr similar to LIST/CONS that
// keeps string with a shared_ptr to avoid copy.
// it will be slightly less efficient to access, but
// will be fast under set!  or better, have a shared_str
// -- remember we do not string mutate operations.
struct VarList;
struct VarNonlist;

using Var = std::variant<VarNum, VarString, VarList, VarNonlist>;
using EnvEntry = std::shared_ptr<Var>;
using GlobalEnv = std::map<int, EnvEntry>;
using FunEnv = std::vector<EnvEntry>;

struct VarList { std::vector<EnvEntry> v; };
struct VarNonlist { std::vector<EnvEntry> v; };

} // ns

#endif
