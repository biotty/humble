#ifndef HUMBLE_UTF
#define HUMBLE_UTF
#include <string>
#include <string_view>

namespace humble {

struct glyph { std::string_view u; };

glyph utf_ref(std::string_view s, size_t i);
long long utf_value(glyph u);

} // ns

#endif
