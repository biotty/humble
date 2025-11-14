#include <string>
#include <exception>

namespace humble {

class Error : std::exception {
    std::string msg;
public:
    Error(std::string m) : msg(m) {};
    const char * what() const noexcept override {
        return msg.c_str();
    }
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

