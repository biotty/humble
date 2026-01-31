#include "fun_impl.hpp"
#include <vector>

using namespace std;

namespace humble {

Names * u_names;

VarExt & vext_or_fail(const vector<int> & ts, span<EnvEntry> args, size_t i, string s)
{
    ostringstream oss;
    oss << s << " args[" << i << "] ";
    if (not holds_alternative<VarExt>(*args[i])) {
        oss << var_type_name(*args[i]);
        throw RunError(oss.str());
    } else if (auto u = get<VarExt>(*args[i]).t;
            find(ts.begin(), ts.end(), u) == ts.end()) {
        oss << "ext:" << u_names->get(u);
        throw RunError(oss.str());
    }
    return get<VarExt>(*args[i]);
}

}
