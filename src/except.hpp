#ifndef HUMBLE_API
#define HUMBLE_API

#include <string>
#ifndef HUMBLE_NOEXC
#include <stdexcept>
#ifdef DEBUG
#include <stacktrace>
#endif
#endif

namespace humble {

#ifndef HUMBLE_NOEXC
struct Error : std::runtime_error
{
    Error(const char * s);
    Error(const std::string & s);
#ifdef DEBUG
    std::stacktrace trace;
#endif
    const char * what() const noexcept override;
};

struct CoreError : Error { using Error::Error; };
struct SrcError : Error { using Error::Error; };
struct RunError : Error { using Error::Error; };
#endif

[[noreturn]] void throwCoreError(const std::string & what);
[[noreturn]] void throwSrcError(const std::string & what);
[[noreturn]] void throwRunError(const std::string & what);

} // ns

#endif
