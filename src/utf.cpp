#include "utf.hpp"
#include "api.hpp"

#ifdef DEBUG
#include "debug.hpp"
#endif

using namespace std;

namespace {

using namespace humble;

Glyph utf_head(std::string_view s)
{
    unsigned u = static_cast<unsigned char>(s[0]);
    if (u < 0b10000000) {
        return { s.substr(0, 1) };
    }
    if (u < 0b11000000) {
        throw SrcError("midst utf8");
    }
    if (u < 0b11100000) {
        return { s.substr(0, 2) };
    }
    if (u < 0b11110000) {
        return { s.substr(0, 3) };
    }
    if (u < 0b11111000) {
        return { s.substr(0, 4) };
    }
    throw SrcError("invalid utf8");
}

} // ans

namespace humble {

long long utf_value(Glyph s)
{
    auto n = s.u.size();
    auto p = reinterpret_cast<const unsigned char *>(s.u.data());
    if (n == 1) return p[0];
    if (n == 2) return
        (p[0] & 0b11111) << 6
        | (p[1] & 0b111111);
    if (n == 3) return
        (p[0] & 0b1111) << 12
        | (p[1] & 0b111111) << 6
        | (p[2] & 0b111111);
    if (n == 4) return
        (p[0] & 0b111) << 18
        | (p[1] & 0b111111) << 12
        | (p[2] & 0b111111) << 6
        | (p[3] & 0b111111);
    throw CoreError("not utf8");
}

Glyph utf_ref(std::string_view s, size_t i)
{
    Glyph t;
    for (;; --i) {
        t = utf_head(s);
        if (i) s = s.substr(t.u.size());
        else break;
    }
#ifdef DEBUG
    cout << "utf: " << t << "\n";
#endif
    return t;
}

} // ns
