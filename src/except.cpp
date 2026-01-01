#include "except.hpp"

using namespace std;

namespace humble {

Error::Error(const char * s)
    : runtime_error(s)
#ifdef DEBUG
    , trace{ stacktrace::current() }
#endif
{ }

Error::Error(const string & s)
    : runtime_error(s)
#ifdef DEBUG
    , trace{ stacktrace::current() }
#endif
{ }

const char * Error::what() const noexcept
{
#ifdef DEBUG
    static std::string buf;
    std::ostringstream ost;
    ost << runtime_error::what() << "\n"
        << trace << "\n";
    buf = ost.str();
    return buf.c_str();
#else
    return runtime_error::what();
#endif
}

} // ns

