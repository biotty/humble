#include "tok.hpp"
#include "except.hpp"
#include "utf.hpp"
#include <cctype>
#include <cstring>
#include <climits>
#include <vector>
#include <list>
#include <charconv>
#include <sstream>
#include <ostream>

#ifdef DEBUG
#include "debug.hpp"
#endif

using namespace std;

namespace humble {

int linenumber;
int wisp_mode;

string_view trim(string_view w)
{
    const auto n = w.size();
    if (n == 0) return w;
    size_t i = 0;
    while (isspace(w[i]))
        if (++i == n) return w.substr(n);
    size_t j = i;
    while (++j != n and not isspace(w[j]))
        ;
    return w.substr(i, j - i);
}

void handle_pragma(string_view w)
{
    w = trim(w);

    if (w == "wisp") wisp_mode = 1;
    else if (w == "no-wisp") wisp_mode = 0;
    else if (w == "wisp-tabs") wisp_mode = 2;
}

#define QUOTES "'`,"
static const char * quotes = QUOTES;
static const char * par_beg_end = PAR_BEG PAR_END;
static const char * name_cs = "!$%&*+-./:<=>?@^_~";

[[noreturn]] void throwSrcErrorLine(const std::string & what)
{
    ostringstream oss;
    oss << "line " << linenumber << ": " << what;
    throwSrcError(oss.str());
}

pair<size_t, optional<size_t>> spaces(const char * s, size_t n)
{
    if (not s) throwCoreError("spaces null");
    auto h = s;
    auto p = s + n;
    optional<size_t> x;
    while (s != p) {
        if (isspace(*s)) {
            if (*s == '\n') {
                ++linenumber;
                x = s - h;
            }
            ++s;
        } else if (*s == ';') {
            // comment
            while (s != p and *s != '\n') ++s;
        } else if (*s == '#') {
            if (s + 1 == p) break;
            if (s[1] == '|') {
                // multiline-comment
                s += 2;
                char x = 0;
                while (s != p) {
                    char y = *s;
                    if (y == '\n') ++linenumber;
                    else if (y == '#' and x == '|') {
                        ++s;
                        break;
                    }
                    x = y;
                    ++s;
                }
                if (s == p) throwSrcErrorLine("#| comment not ended");
            } else if (s[1] == '!') {
                s += 2;
                auto z = s;
                while (s != p and *s != '\n')
                    ++s;
                if (*s != '\n') throwSrcErrorLine("#! with no end");
                handle_pragma({z, static_cast<size_t>(s - z)});
            } else break;  // sole # token
        } else break;
    }
    return {s - h, x};
}

pair<string_view, size_t> tok(std::string_view s)
{
    if (s.empty()) throwCoreError("tok empty");
    size_t i = 0;
    auto r = [s, &i]() -> decltype(tok("")) {
        return {s.substr(0, i), i};
    };
    if (strchr(par_beg_end, s[i])) {
        ++i;
        return r();
    }
    const auto n = s.size();
    if (s[i] == '#') {
        if (++i == n) throwSrcErrorLine("stop at #");
        if (s[i] == '\\' and ++i == n)
            throwSrcErrorLine("stop at #\\");
        if (isspace(s[i])) throwSrcErrorLine("# space");
        if (isalnum(s[i])) while (++i != n and isalnum(s[i]));
        else {
            auto w = utf_ref(s.substr(i), 0);
            i += w.u.size();
        };
        return r();
    }
    if (s[i] == '"') {
        while (i < n) {
            if (++i == n) throwSrcErrorLine("stop in string");
            if (s[i] == '"') {
                ++i;
                return r();
            }
            if (s[i] == '\\') {
                if (++i == n) throwSrcErrorLine("stop in string at '\\'");
            }
        }
    }
    if (s[i] == '@') {
        if (++i == n) throwSrcErrorLine("stop at @");
        return r();
    }
    if (strchr(quotes, s[i])) {
        if (++i == n) throwSrcErrorLine("stop at quote");
        return r();
    }
    while (isalnum(s[i]) or strchr(name_cs, s[i])) {
        if (++i == n) break;
    }
    if (i) return r();
    auto g = utf_ref(s, 0);
    throwSrcErrorLine("glyph '" + string(g.u) + "'");
}

string unescape_string(string_view s)
{
    vector<char> r;
    const auto n = s.size();
    for (size_t i = 0; i < n; ++i) {
        char c = s[i];
        if (c == '\\') {
            if (++i == n) throwSrcErrorLine("end at '\\' in string");
            switch (s[i]) {
                case 't': c = '\t'; break;
                case 'n': c = '\n'; break;
                case 'r': c = '\r'; break;
                case '"': c = '\"'; break;
                case '0': case '1': case '2': case '3': case '4':
                case '5': case '6': case '7':
                          {
                              short b = s[i] - '0';
                              while (++i != n) {
                                  c = s[i];
                                  if (c < '0' or '7' < c)
                                      break;
                                  b *= 8;
                                  b += c - '0';
                                  if (b > 255)
                                      throwSrcErrorLine("octal overflow");
                              }
                              c = b;
                          }
                          break;
                default:
                          throwSrcErrorLine("invalid string escape");
            }
        }
        r.push_back(c);
    }
    return { r.begin(), r.end() };
}

Names::Names() {}

Names::Names(std::initializer_list<std::string> w)
{
    for (auto s : w) intern(s);
}

size_t Names::size() { return v.size(); }

int Names::add(std::string_view name, size_t h)
{
    auto i = v.size();
    if (i == INT_MAX) throwCoreError("names overflow");
    int r = i;
    v.emplace_back(name);
    m.insert({h, r});
    return r;
}

int Names::intern(std::string_view name)
{
    auto h = hasher(name);
    for (auto [b, e] = m.equal_range(h); b != e; ++b) {
        if (v[b->second] == name) return b->second;
    }
    return add(name, h);
}

std::string Names::get(int h)
{
    if (static_cast<size_t>(h) >= v.size()) {
        ostringstream oss;
        oss << '[';
        if (v.size() != 0) oss << v.size() << "<=";
        oss << h << ']';
        return oss.str();
    }
    return v.at(h);
}

class Frontend {
    const string_view s;
    size_t i = 0;
    bool f = true;
    vector<int> w = {-1};
    list<string_view> p;
    int c = 0;

    string_view unqueue()
    {
        auto r = p.front();
        p.erase(p.begin());
        return r;
    }

    string_view esctok()
    {
        auto u = s.substr(i);
        auto [k, x] = spaces(u.data(), u.size());
        if (x.has_value())
            throwSrcErrorLine("newline after wisp-dot");
        u = u.substr(k);
        i += k;
        if (u.empty()) throwSrcErrorLine("eof at wisp-dot");
        auto [t, j] = tok(u);
        i += j;
        return t;
    }

    static constexpr string c_beg = "(";
    static constexpr string c_end = ")";
    static constexpr string w_beg = "[";
    static constexpr string w_end = "]";

public:

    Frontend(string_view s) : s(s) {}

    string_view get()
    {
        if (not p.empty())
            return unqueue();
        auto y = i;
        auto u = s.substr(i);
        auto [k, x] = spaces(u.data(), u.size());
        u = u.substr(k);
        i += k;
        if (u.empty()) {
            for (int h = 0; h != c; ++h)
                p.push_back(c_end);
            for (size_t q = 1; q != w.size(); ++q)
                p.push_back(w_end);
            p.push_back(u);
            return get();
        }
        auto [t, j] = tok(u);
        auto b = i;
        i += j;
        if (not wisp_mode)
            return t;
        if (f) {
            f = false;
            if (not x.has_value()) x = -1;
        }
        if (not x.has_value()) {
            if (t != ":") return t;
            ++c;
            return c_beg;
        }
        auto z = static_cast<ptrdiff_t>(b) - 1
            - static_cast<ptrdiff_t>(y + x.value());
        if (memchr(u.data() - z, (wisp_mode == 1 ? '\t' : ' '), z))
            throwSrcErrorLine("disallowed indent for wisp mode");
        bool escdot = (t == ".");
        if (escdot) t = esctok();
        bool start = escdot ^ ! strchr(PAR_BEG PAR_END QUOTES, t[0]);
        for (; c > 0; --c) p.push_back(c_end);
        if (t == ":") {
            ++c;
            t = c_beg;
        }
        while (z <= w.back()) {
            w.pop_back();
            p.push_back(w_end);
        }
        if (start) {
            w.push_back(z);
            p.push_back(w_beg);
        }
        p.push_back(t);
        return unqueue();
    }
};

// Function scans to produce tokens such as numeric
// literals, strings and names.  The names are
// "interned", meaning each given an identifier.
vector<Lex> lex(const string & s, Names & names)
{
    vector<Lex> r;
    Frontend f{s};
    while (true) {
        const auto t = f.get();
        if (t.empty()) break;
        Lex v;
        if (auto p = strchr(par_beg_end, t[0]); p) {
            auto par = static_cast<int>(p - par_beg_end);
            if (par < 3) v = LexBeg{par};
            else v = LexEnd{par - 3, linenumber};
        } else if (isdigit(t[0]) or (t.size() != 1
                    and (t[0] == '-' or t[0] == '+'))) {
            long long i;
            if (from_chars(t.data(), t.data() + t.size(), i).ec != errc{})
                throwSrcErrorLine("number");
            v = LexNum{i};
        } else if (t[0] == '#') {
            if (t == "#t" or t == "#f" or t == "#true" or t == "#false") {
                v = LexBool{t[1] == 't'};
            } else if (strchr("bodx", t[1])) {
                int base;
                switch (t[1]) {
                    case 'b': base = 2; break;
                    case 'o': base = 8; break;
                    case 'd': base = 10; break;
                    case 'x': base = 16; break;
                    default: abort();
                }
                long long i;
                if (from_chars(t.data() + 2,
                            t.data() + t.size(), i, base).ec != errc{})
                    throwSrcErrorLine("# numeric");
                v = LexNum{i};
            } else if (t[1] == '\\') {
                auto w = utf_ref(t, 2);
                if (t.size() == 2 + w.u.size()) {
                    v = LexNum{utf_value(w)};
                } else {
                    int i;
                    auto s = t.substr(2);
                    if (s == "alarm") i = 7;
                    else if (s == "backspace") i = 8;
                    else if (s == "tab") i = 9;
                    else if (s == "newline") i = 10;
                    else if (s == "return") i = 13;
                    else if (s == "escape") i = 27;
                    else if (s == "space") i = 32;
                    else if (s == "delete") i = 127;
                    else throwSrcErrorLine("#\\");
                    v = LexNum{i};
                }
            } else if (t == "#void") {
                v = LexVoid{};
            } else if (t == "#r") {
                v = LexR{};
            } else throwSrcErrorLine("# token");
        } else if (t[0] == '"') {
            v = LexString{ unescape_string(t.substr(1, t.size() - 2)) };
        } else if (t[0] == '.') {
            if (t.size() != 1) throwSrcErrorLine("token starts in '.'");
            v = LexDot{};
        } else if (t[0] == '@') {
            if (t.size() != 1) throwSrcErrorLine("token starts in '@'");
            v = LexSpl{};
        } else if (t[0] == quotes[0]) {
            v = LexQt{};
        } else if (t[0] == quotes[1]) {
            v = LexQqt{};
        } else if (t[0] == quotes[2]) {
            v = LexUnq{};
        } else {
            v = LexNam{names.intern(t), linenumber};
        }
        r.push_back(v);
    }

#ifdef DEBUG
        cout << "lex: " << r << "\n";
#endif
    return r;
}

string escape(string s)
{
    size_t p{};
    while ((p = s.find("\"", p)) != s.npos) {
        s.replace(p, 1, "\\\"");
        p += 2;
    }
    return s;
}

span<Lex> span1(span<Lex> x, size_t i)
{
    return {x.begin() + i, x.begin() + i + 1};
}

} // ns

