#include "tok.hpp"
#include "api.hpp"
#include <cctype>

using namespace std;

namespace humble {

string filename;
int linenumber;

const char * spaces(const char * s)
{
    if ( ! s) throw CoreError("spaces null");
    while (s[0]) {
        if (isspace(s[0])) {
            if (s[0] == '\n') linenumber += 1;
            s += 1;
        } else if (s[0] == ';') {
            // comment
            while (s[0] != '\n') s += 1;
        } else if (s[0] == '#') {
            if (s[1] and s[1] == '|') {
                // multiline-comment
                s += 2;
                char x = 0;
                while (s[0]) {
                    char y = s[0];
                    if (y == '\n') linenumber += 1;
                    else if (y == '#' and x == '|') {
                        s += 1;
                        break;
                    }
                    x = y;
                    s += 1;
                }
                if ( ! s[0]) throw
                    SrcError("#| comment not ended");
            } else break;
        } else break;
    }
    return s;
}

} // ns

