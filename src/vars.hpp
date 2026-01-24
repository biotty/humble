#ifndef HUMBLE_VARS
#define HUMBLE_VARS

#include <string>
#include <vector>
#include <variant>
#include <map>
#include <set>
#include <memory>
#include <initializer_list>
#include <span>
#include <iosfwd>

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
struct VarRec;
struct VarSplice;
struct FunOps;
struct VarFunOps { std::shared_ptr<FunOps> f; };
struct VarFunHost;
struct VarApply;
struct Cons;
struct VarCons { std::shared_ptr<Cons> c; };

using Var = std::variant<VarVoid, VarNum, VarBool, VarNam, VarString/*4*/,
      VarList, VarNonlist, VarSplice, VarUnquote/*8*/,
      VarFunOps, VarFunHost, VarApply, VarCons/*12*/, VarRec>;
using EnvEntry = std::shared_ptr<Var>;

struct VarList { std::vector<EnvEntry> v; };
struct VarNonlist { std::vector<EnvEntry> v; };
struct VarRec { std::vector<EnvEntry> v; };
struct VarSplice { std::vector<EnvEntry> v; };
typedef EnvEntry (*FunHost)(std::span<EnvEntry> a);
struct VarFunHost { FunHost p; };
struct VarApply { std::vector<EnvEntry> a; };

const char * var_type_name(const Var & v);

struct Env {
    virtual EnvEntry get(int i) = 0;
    virtual void set(int i, EnvEntry e) = 0;
    virtual ~Env() = default;
};

struct GlobalEnv : Env {
    struct create_t { };
    // note: ^ control construction as primary use is singleton.
    // alternative: split this class in two (secondary use is
    // for testing and in the overlay env implementation)
    static GlobalEnv & instance();  // get global aka "initial" env
    GlobalEnv(create_t);
    GlobalEnv(Env &) = delete;
    bool operator=(Env &) = delete;
    EnvEntry get(int i) final override;
    void set(int i, EnvEntry e) final override;
    GlobalEnv init();
    std::set<int> keys();
private:
    bool initial;
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
    explicit FunEnv(size_t n);
    explicit FunEnv(std::initializer_list<EnvEntry> v);
    EnvEntry get(int i) override;
    void set(int i, EnvEntry e) override;
    friend LexEnv;
private:
    std::vector<EnvEntry> v;
};

struct NullEnv : Env {
    EnvEntry get(int) override;
    void set(int, EnvEntry);
};

inline NullEnv nullenv;

} // ns

#endif
