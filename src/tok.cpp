#include "tok.hpp"
#include "api.hpp"
#include <cctype>
#include <cstring>
#include <vector>
#include <algorithm>
#include <charconv>

using namespace std;

namespace humble {

string filename;
int linenumber;

const char * par_beg_end =
    "([{"
    ")]}";
const char * quotes =
    "'`,";
const char * name_cs = "!$%&*+-./:<=>?@^_~";

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
            while (s[0] != '\n') s += 1;
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
        if (++i == n) throw SrcError("terminated at #");
        if (s[i] == '\\' and ++i == n)
            throw SrcError("terminated at #\\");
        if (isspace(s[i])) throw SrcError("# space");
        if (isalnum(s[i])) while (++i != n and isalnum(s[i]));
        else i += 1;
        return r();
    }
    if (s[i] == '"') {
        while (i < n) {
            if (++i == n) throw SrcError("terminated in string");
            if (s[i] == '"') {
                ++i;
                return r();
            }
            if (s[i] == '\\') {
                if (++i == n) throw SrcError("terminated in string at '\\'");
            }
        }
    }
    if (s[i] == '@') {
        if (++i == n) throw SrcError("terminated at @");
        return r();
    }
    if (strchr(quotes, s[i])) {
        if (++i == n) throw SrcError("terminated at quote");
        return r();
    }
    while (isalnum(s[i]) or strchr(name_cs, s[i])) {
        if (++i == n) break;
    }
    if (i) return r();
    static char m[16];
    snprintf(m, sizeof m, "character %d", s[i]);
    // improve: verbatim complete utf8 entity if available
    // (maybe one byte) -- or otherwise first byte numeric
    throw SrcError(m);
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
                case '0': case '1': case '2': case '3': case '4':
                case '5': case '6': case '7':
                          {
                              short b = s[i] - '0';
                              while (++i != n) {
                                  c = s[i];
                                  if (not strchr("01234567", c))
                                      break;
                                  b *= 8;
                                  b += c - '0';
                                  if (b > 255)
                                      throw SrcError("bad octal in string");
                              }
                              c = b;
                          }
                          break;
                default:
                          throw SrcError("invalid string escape");
            }
        r.push_back(c);
        }
    }
    return { r.begin(), r.end() };
}

size_t intern(std::string_view name, Names & names)
{
    auto t = find(names.begin(), names.end(), name);
    if (t != names.end())
        return (distance(names.begin(), t));
    auto r = names.size();
    names.emplace_back(name);
    return r;
}

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
        auto [t, k] = tok(string_view(s).substr(i));
        if (t.empty()) break;
        i += k;
        Lex v;
        if (auto p = strchr(par_beg_end, t[0]); p) {
            auto par = static_cast<int>(p - par_beg_end);
            if (par < 3) v = LexBeg{par};
            else v = LexEnd{par - 3, linenumber};
        } else if (isdigit(t[0]) or (t.size() != 1 and strchr("-+.", t[0]))) {
            long long i;
            from_chars(t.data(), t.data() + t.size(), i);
            v = LexNum{i};
        }/*
        elif t[0] == "#":
            if t[1] in "tf":
                if len(t) == 2 or t[1:] in ("true", "false"):
                    v = (LEX_BOOL, "t" == t[1])
                else:
                    raise SrcError("'%s' at %d" % (linenumber,))
            elif t[1] in "bodx":
                base = [2, 8, 10, 16]["bodx".index(t[1])]
                try:
                    v = (LEX_NUM, int(t[2:], base))
                except ValueError:
                    raise SrcError("'%s' at %d" % (linenumber,))
            elif t[1] == "\\":
                if len(t) == 3:
                    v = (LEX_NUM, ord(t[2]))
                else:
                    v = None
                    for m, c in [("alarm", 7), ("backspace", 8), ("tab", 9),
                            ("newline", 10), ("return", 13), ("escape", 27),
                            ("space", 32), ("delete", 127)]:
                        if t[2:] == m:
                            v = (LEX_NUM, c)
                            break
                    if v is None:
                        raise SrcError("#\ token at line %d" % (linenumber,))
            elif t == "#void":
                v = (LEX_VOID,)
            else:
                raise SrcError("# token at line %d" % (linenumber,))
        elif t[0] == "\"":
            v = (LEX_STRING, unescape_string(t[1:-1]))
        elif t.startswith("."):
            assert len(t) == 1
            v = (LEX_DOT,)
        elif t.startswith("@"):
            assert len(t) == 1
            v = (LEX_SPL,)
        elif t[0] in quotes:
            assert len(t) == 1
            v = (quotes.index(t[0]) + LEX_QT,)
        else:
            h = intern(t, names)
            v = (LEX_NAM, h, linenumber)
        */
        r.push_back(v);
    }
    //debug("lex", a)
    return r;
}

} // ns

