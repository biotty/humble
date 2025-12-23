#ifndef HUMBLE_API
#define HUMBLE_API

#include <stdexcept>

namespace humble {

struct Error : std::runtime_error
{
    using std::runtime_error::runtime_error;
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
