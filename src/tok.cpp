#include "tok.hpp"
#include "except.hpp"
#include "utf.hpp"
#include <cctype>
#include <cstring>
#include <climits>
#include <vector>
#include <charconv>
#include <sstream>
#include <ostream>

#ifdef DEBUG
#include "debug.hpp"
#endif

using namespace std;

namespace humble {

int linenumber;

static const char * quotes = "'`,";
static const char * par_beg_end = PAR_BEG PAR_END;
static const char * name_cs = "!$%&*+-./:<=>?@^_~";

size_t spaces(const char * s, size_t n)
{
    if ( ! s) throw CoreError("spaces null");
    auto h = s;
    auto p = s + n;
    while (s != p) {
        if (isspace(s[0])) {
            if (s[0] == '\n') linenumber += 1;
            s += 1;
        } else if (s[0] == ';') {
            // comment
            while (s != p and s[0] != '\n') s += 1;
        } else if (s[0] == '#') {
            if (s + 1 != p and s[1] == '|') {
                // multiline-comment
                s += 2;
                char x = 0;
                while (s != p) {
                    char y = s[0];
                    if (y == '\n') linenumber += 1;
                    else if (y == '#' and x == '|') {
                        s += 1;
                        break;
                    }
                    x = y;
                    s += 1;
                }
                if (s == p) throw
                    SrcError("#| comment not ended");
            } else break;
        } else break;
    }
    return s - h;
}

pair<string_view, size_t> tok(std::string_view s)
{
    if (s.empty()) throw CoreError("tok empty");
    auto k = spaces(s.data(), s.size());
    s = s.substr(k);
    if (s.empty()) return {s, k};
    size_t i = 0;
    auto r = [s, &i, k]() -> decltype(tok("")) {
        return {s.substr(0, i), k + i};
    };
    if (strchr(par_beg_end, s[i])) {
        ++i;
        return r();
    }
    const auto n = s.size();
    if (s[i] == '#') {
        if (++i == n) throw SrcError("stop at #");
        if (s[i] == '\\' and ++i == n)
            throw SrcError("stop at #\\");
        if (isspace(s[i])) throw SrcError("# space");
        if (isalnum(s[i])) while (++i != n and isalnum(s[i]));
        else {
            auto w = utf_ref(s.substr(i), 0);
            i += w.u.size();
        };
        return r();
    }
    if (s[i] == '"') {
        while (i < n) {
            if (++i == n) throw SrcError("stop in string");
            if (s[i] == '"') {
                ++i;
                return r();
            }
            if (s[i] == '\\') {
                if (++i == n) throw SrcError("stop in string at '\\'");
            }
        }
    }
    if (s[i] == '@') {
        if (++i == n) throw SrcError("stop at @");
        return r();
    }
    if (strchr(quotes, s[i])) {
        if (++i == n) throw SrcError("stop at quote");
        return r();
    }
    while (isalnum(s[i]) or strchr(name_cs, s[i])) {
        if (++i == n) break;
    }
    if (i) return r();
    auto g = utf_ref(s.substr(i), 0);
    throw SrcError("glyph '" + string(g.u) + "'");
}

string unescape_string(string_view s)
{
    vector<char> r;
    const auto n = s.size();
    for (size_t i = 0; i < n; ++i) {
        char c = s[i];
        if (c == '\\') {
            if (++i == n) throw SrcError("end at '\\' in string");
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
                                      throw SrcError("octal overflow");
                              }
                              c = b;
                          }
                          break;
                default:
                          throw SrcError("invalid string escape");
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
    if (i == INT_MAX) throw SrcError("names overflow");
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
        oss << "#[" << h << "]";
        return oss.str();
    }
    return v.at(h);
}

// Function scans to produce tokens such as numeric
// literals, strings and names.  The names are
// "interned", meaning each given an identifier.
vector<Lex> lex(const string & s, Names & names)
{
    const auto n = s.size();
    size_t i = 0;
    if (s[0] == '#' and s[1] == '!') {
        i = s.find('\n');
        if (i == s.npos) throw SrcError("'#!' without end");
    }
    vector<Lex> r;
    while (i != n) {
        const auto [t, k] = tok(string_view(s).substr(i));
        if (t.empty()) break;
        i += k;
        Lex v;
        if (auto p = strchr(par_beg_end, t[0]); p) {
            auto par = static_cast<int>(p - par_beg_end);
            if (par < 3) v = LexBeg{par};
            else v = LexEnd{par - 3, linenumber};
        } else if (isdigit(t[0]) or (t.size() != 1
                    and (t[0] == '-' or t[0] == '+'))) {
            long long i;
            if (from_chars(t.data(), t.data() + t.size(), i).ec != errc{})
                throw SrcError("number");
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
                    throw SrcError("# numeric");
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
                    else throw SrcError("#\\");
                    v = LexNum{i};
                }
            } else if (t == "#void") {
                v = LexVoid{};
            } else if (t == "#r") {
                v = LexR{};
            } else throw SrcError("# token");
        } else if (t[0] == '"') {
            v = LexString{ unescape_string(t.substr(1, t.size() - 2)) };
        } else if (t[0] == '.') {
            if (t.size() != 1) throw SrcError("token starts in '.'");
            v = LexDot{};
        } else if (t[0] == '@') {
            if (t.size() != 1) throw SrcError("token starts in '@'");
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

