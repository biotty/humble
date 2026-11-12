#include "except.hpp"
#include "tok.hpp"
#include <sstream>
#ifdef HUMBLE_NOEXC
#include <cstdlib>
#include <iostream>
#include "trace.hpp"
#endif

using namespace std;

namespace humble {

extern Names * u_names;
extern LexNam expandsname;

#ifndef HUMBLE_NOEXC
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
    static string buf;
    ostringstream ost;
    ost << runtime_error::what() << "\n"
        << trace << "\n";
    buf = ost.str();
    return buf.c_str();
#else
    return runtime_error::what();
#endif
}
#else
[[noreturn]] void err(const string & what)
{
    cerr << what << endl;
    trace_print_all(cerr);
    exit(EXIT_FAILURE);
}
#endif

[[noreturn]] void throwCoreError(const string & what)
{
#ifndef HUMBLE_NOEXC
    throw CoreError(what);
#else
    err(what);
#endif
}

[[noreturn]] void throwSrcError(const string & what)
{
    struct { [[noreturn]] void operator()(const string & s) {
#ifndef HUMBLE_NOEXC
            throw SrcError(s);
#else
            err(s);
#endif
    } } f;
    const auto & n = expandsname;
    if (n.line == 0) {
        f(what);
    }
    ostringstream os;
    os << u_names->get(n.h)
        << " at line " << n.line << ": " << what;
    f(os.str());
}

[[noreturn]] void throwRunError(const string & what)
{
#ifndef HUMBLE_NOEXC
    throw RunError(what);
#else
    err(what);
#endif
}

} // ns

