#include "fun_impl.hpp"
#include "compx.hpp"
#include <iostream>
#include <cstring>

using namespace humble;
using namespace std;

namespace {

constexpr size_t trace_n = 32;
uintptr_t trace[trace_n];
size_t cursor;

void regress(size_t & i)
{
    if (--i >= trace_n) i += trace_n;
}

void advance(size_t & i)
{
    if (++i >= trace_n) i = 0;
}

size_t next(size_t & i)
{
    auto r = i;
    advance(i);
    return r;
}

size_t first(size_t i)
{
    if (trace[i] == 0) {
        if (i == 0) return trace_n;
        return 0;
    }
    while ((trace[i] & 1) == 0) advance(i);
    return i;
}

size_t last(size_t i)
{
    if (trace[i] == 0 and i == 0) return trace_n;
    do { regress(i); } while ((trace[i] & 1) == 0);
    return i;
}

void trace_print(size_t & i, ostream & os)
{
    Lex & x = *reinterpret_cast<Lex *>(trace[next(i)] & ~1);
    if (holds_alternative<LexNam>(x)) {
        LexEnv * le = reinterpret_cast<LexEnv *>(trace[next(i)]);
        os << ' ';
        int h = get<LexNam>(x).h;
        if (le) h = le->name(h);
        if (h < 0) {
            h = -h;
            os << '-';
        }
        os << u_names->get(h);
    } else if (holds_alternative<LexSym>(x)
            or holds_alternative<LexNum>(x)
            or holds_alternative<LexBool>(x)
            or holds_alternative<LexString>(x)) {
        auto v = from_lex(x);
        os << ' ';
        print(v, *u_names, os);
    } else if (holds_alternative<LexOp>(x)) {
        auto op = get<LexOp>(x);
        if (op.code == OP_BIND) {
            os << " bind";
        } else if (op.code == OP_LAMBDA or op.code == OP_LAMBDA_DOT) {
            os << " lambda";
        } else if (op.code == OP_COND) {
            os << " cond";
        } else if (op.code == OP_IMPORT) {
            os << " import";
        } else if (op.code == OP_SEQ) {
            os << " seq";
        }
    }
}

} // ans

namespace humble {

void trace_reset()
{
    cursor = 0;
    memset(trace, 0, sizeof trace);
}

bool trace_put(Lex & x, LexEnv * le)
{
    if (not (holds_alternative<LexNam>(x)
            or holds_alternative<LexSym>(x)
            or holds_alternative<LexNum>(x)
            or holds_alternative<LexBool>(x)
            or holds_alternative<LexString>(x)))
        return false;

    trace[next(cursor)] = 1 | reinterpret_cast<uintptr_t>(&x);
    if (holds_alternative<LexNam>(x)) {
        trace[next(cursor)] = reinterpret_cast<uintptr_t>(le);
    }
    return true;
}

void trace_print_last(ostream & os)
{
    auto i = last(cursor);
    if (i == trace_n) return;
    trace_print(i, os);
}

void trace_print_all(ostream & os)
{
    auto i = first(cursor);
    os << "trace:";
    if (i != trace_n)
        do { trace_print(i, os); } while (i != cursor);
    os << "\n";
}

} // ns

