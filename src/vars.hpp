#ifndef HUMBLE_VARS
#define HUMBLE_VARS

#include <string>
#include <vector>
#include <variant>
#include <map>
#include <memory>
#include <initializer_list>

namespace humble {

struct VarVoid { };
struct VarNum { long long i; };
struct VarBool { bool b; };
struct VarNam { int h; };
struct VarString { std::string s; };
// idea: have BigStr similar to LIST/CONS that
// keeps string with a shared_ptr to avoid copy.
// it will be slightly less efficient to access, but
// will be fast under setv! or better, have a shared_str
// -- remember we do no operation that mutate strings.

struct LexUnquote;
struct VarUnquote{ LexUnquote * u; };
struct VarList;
struct VarNonlist;
struct VarSplice;

using Var = std::variant<VarVoid, VarNum, VarBool, VarNam, VarString,
      VarList, VarNonlist, VarSplice, VarUnquote>;
using EnvEntry = std::shared_ptr<Var>;

struct VarList { std::vector<EnvEntry> v; };
struct VarNonlist { std::vector<EnvEntry> v; };
struct VarSplice { std::vector<EnvEntry> v; };

struct Env {
    virtual EnvEntry get(int i) = 0;
    virtual void set(int i, EnvEntry e) = 0;
};

struct GlobalEnv : Env {
    struct create_t {};
    // note: ^ control construction as primary use is singleton.
    // alternative: split this class in two (secondary use is
    // for testing and in the overlay env implementation)
    static Env & instance();  // get global aka "initial" env
    GlobalEnv(create_t);
    GlobalEnv(Env &) = delete;
    bool operator=(Env &) = delete;
    EnvEntry get(int i) override;
    void set(int i, EnvEntry e) override;
private:
    std::map<int, EnvEntry> m;
};

struct OverlayEnv : Env {
    explicit OverlayEnv(Env & d);
    EnvEntry get(int i) override;
    void set(int i, EnvEntry e) override;
private:
    GlobalEnv m;
    Env & e;
};

struct LexEnv;

struct FunEnv : Env {
    explicit FunEnv(std::initializer_list<EnvEntry> v);
    EnvEntry get(int i) override;
    void set(int i, EnvEntry e) override;
    friend LexEnv;
private:
    explicit FunEnv(size_t n);
    std::vector<EnvEntry> v;
};

} // ns

#endif
