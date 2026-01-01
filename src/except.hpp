#ifndef HUMBLE_API
#define HUMBLE_API

#include <stdexcept>

#ifdef DEBUG
#include <stacktrace>
#endif

namespace humble {

struct Error : std::runtime_error
{
    Error(const char * s);
    Error(const std::string & s);
#ifdef DEBUG
    std::stacktrace trace;
#endif
    const char * what() const noexcept override;
};

struct CoreError : Error {
    using Error::Error;
};

struct SrcError : Error {
    using Error::Error;
};

struct RunError : Error {
    using Error::Error;
};

} // ns

#endif
