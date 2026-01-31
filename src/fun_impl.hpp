#ifndef HUMBLE_FUN_IMPL
#define HUMBLE_FUN_IMPL

#include "tok.hpp"
#include "vars.hpp"
#include "except.hpp"
#include <sstream>

namespace humble {

template <typename T, typename... Ts>
bool valt_in(Var & v)
{
    if (holds_alternative<T>(v)) return true;
    if constexpr (sizeof...(Ts) != 0) return valt_in<Ts...>(v);
    return false;
}

template <typename... Ts>
void valt_or_fail(std::span<EnvEntry> args, size_t i, std::string s)
{
    if (valt_in<Ts...>(*args[i])) return;
    std::ostringstream oss;
    oss << s << " args[" << i << "] " << var_type_name(*args[i]);
    throw RunError(oss.str());
}

extern Names * u_names;

VarExt & vext_or_fail(const std::vector<int> & ts,
        std::span<EnvEntry> args, size_t i, std::string s);

} // ns

#endif
