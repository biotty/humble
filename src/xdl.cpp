#include "xdl.hpp"
#include "xdl_use.hpp"
#include "except.hpp"

// dlopen
#include <dlfcn.h>

using namespace std;
using namespace humble;

LibLoader::LibLoader(string libs_dir) : libs_dir(libs_dir) { }

unique_ptr<Macro> LibLoader::requires_macro()
{
    struct Requires : MacroClone<Requires> {
        set<string> * accum;
        Requires(set<string> & accum) : accum(&accum) { }
        Lex operator()(LexForm && s) override
        {
            if (s.v.size() != 2) throw SrcError("requires argc");
            if (not holds_alternative<LexString>(s.v[1])) throw SrcError("requires");
            accum->insert(get<LexString>(s.v[1]).s);
            return LexVoid{};
        }
    };
    return make_unique<Requires>(requires_accum);
}

static void load_lib(const string & libs_dir, const string & name,
        GlobalEnv & env, Names & n, ostream & es)
{
    xdl_arg u_arg = { &n, &env };
    string path = libs_dir + "/libH" + name + ".so";
    string sym = "xdl_" + name;
    // consider: ^ use mere symbol "init" always, if allows
    void * dl = dlopen(path.c_str(), RTLD_NOW);
    if (not dl) {
        es << dlerror() << endl;
        exit(1);
    } else {
        auto f = (xdl_fn)dlsym(dl, sym.c_str());
        if (not f) {
            es << sym << ": " << dlerror() << endl;
            exit(1);
        } else {
            f(&u_arg);
        }
    }
}

void LibLoader::operator()(GlobalEnv & env, Names & n, ostream & es)
{
    for (const string & s : requires_accum)
        load_lib(libs_dir, s, env, n, es);
}

