#include <string>
#include <string_view>
#include <vector>
#include <utility>
#include <variant>

namespace humble {

extern std::string filename;
extern int linenumber;

size_t spaces(const char * s, size_t n);

std::pair<std::string_view, size_t> tok(std::string_view s);

std::string unescape_string(std::string_view s);

using Names = std::vector<std::string>;

size_t intern(std::string_view name, Names & names);

struct LexBeg{ int par; };
struct LexEnd{ int par; int line; };
struct LexNum{ long long i; };

using Lex = std::variant<
    LexBeg,
    LexEnd,
    LexNum
        >;

std::vector<Lex> lex(const std::string & s, Names & names);

} // ns

