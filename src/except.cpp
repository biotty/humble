#include "except.hpp"

#ifdef HUMBLE_NOEXC
#include <cstdlib>
#include <iostream>
#endif

using namespace std;

namespace humble {

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
#ifndef HUMBLE_NOEXC
    throw SrcError(what);
#else
    err(what);
#endif
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

