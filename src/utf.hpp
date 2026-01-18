#ifndef HUMBLE_UTF
#define HUMBLE_UTF
#include <string>
#include <string_view>

namespace humble {

// Class holds reference to a snippet that
// encodes exactly one unicode abstract character.
// Note that in unicode terminology "glyph"
// would be an incorrect name for what we have.
struct Glyph { std::string_view u; };

Glyph utf_ref(std::string_view s, size_t i);
long long utf_value(Glyph u);
std::string utf_make(long long i);

} // ns

#endif
