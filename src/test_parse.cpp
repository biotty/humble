#include "except.hpp"
#include "parse.hpp"
#include "detail.hpp"
#include "gtest/gtest.h"

using namespace humble;
using namespace std;

TEST(parse, empty)
{
    Names n;
    Macros m;
    ASSERT_EQ(0, parse("", n, m).v.size());
}

TEST(parse, flat)
{
    Names n;
    Macros m;
    ASSERT_EQ(2, parse("1 2", n, m).v.size());
}

TEST(parse, beg_end)
{
    Names n;
    Macros m;
    auto w = parse("()", n, m);
    ASSERT_EQ(0, get<LexForm>(w.v.at(0)).v.size());
}

TEST(parse, beg_end_mismatch)
{
    Names n;
    Macros m;
    ASSERT_THROW(parse("(]", n, m), SrcError);
}

TEST(parse, beg_end_unmatch)
{
    Names n;
    Macros m;
    ASSERT_THROW(parse("{", n, m), SrcError);
    ASSERT_THROW(parse("}", n, m), SrcError);
}

TEST(parse, quote)
{
    Names n = { "a", "b", "c", "d" };
    Macros m;
    auto w = parse("'d", n, m);
    auto f = get<LexForm>(w.v.at(0));
    ASSERT_EQ(3, get<LexNam>(f.v.at(1)).h);
}

TEST(parse, splice)
{
    Names n;
    Macros m;
    auto w = parse("@a", n, m);
    auto s = get<LexSplice>(w.v.at(0));
    ASSERT_TRUE(holds_alternative<LexNam>(s.v.at(0)));
}

TEST(expand, quote_list)
{
    Names n = init_names();
    Macros m = qt_macros();
    auto w = parse("'(1 \"two\" three)", n, m);
    auto s = get<LexList>(w.v.at(0));
    ASSERT_TRUE(holds_alternative<LexSym>(s.v.at(2)));
}

TEST(expand, quote_nonlist)
{
    Names n = init_names();
    Macros m = qt_macros();
    auto w = parse("'(1 \"two\" . three)", n, m);
    auto s = get<LexNonlist>(w.v.at(0));
    ASSERT_TRUE(holds_alternative<LexSym>(s.v.at(2)));
}

TEST(expand, unquote_quasiquote)
{
    Names n = init_names();
    Macros m = qt_macros();
    auto w = parse("`(foo ,bar)", n, m);
    ASSERT_EQ(1, w.v.size());
    auto s = get<LexList>(w.v.at(0));
    ASSERT_TRUE(holds_alternative<LexSym>(s.v.at(0)));
    ASSERT_TRUE(holds_alternative<LexNam>(s.v.at(1)));
}

TEST(expand, top_form)
{
    Names n = init_names();
    Macros m = qt_macros();
    auto w = parse("quote name", n, m);
    ASSERT_EQ(2, w.v.size());
}

TEST(expand, twice_quasiquote)
{
    Names n = init_names();
    Macros m = qt_macros();
    int h = n.size();
    auto w = parse("`(a `(b ,,name1 ,',name2 d) e)", n, m);
    // (a (quasiquote (b (unquote N1) (unquote (quote N2)) d)) e)
    auto t = get<LexList>(w.v.at(0));
    auto a = get<LexSym>(t.v.at(0));
    ASSERT_EQ(h, a.h);
    auto qq_expr = get<LexList>(t.v.at(1));
    auto qq = get<LexSym>(qq_expr.v.at(0));
    ASSERT_EQ(NAM_QUASIQUOTE, qq.h);
    auto qq_arg = get<LexList>(qq_expr.v.at(1));
    auto b = get<LexSym>(qq_arg.v.at(0));
    ASSERT_EQ(h + 1, b.h);
    auto unq1_expr = get<LexList>(qq_arg.v.at(1));
    auto unq1 = get<LexSym>(unq1_expr.v.at(0));
    ASSERT_EQ(NAM_UNQUOTE, unq1.h);
    auto name1 = get<LexNam>(unq1_expr.v.at(1));
    ASSERT_EQ(h + 2, name1.h);
    auto unq2_expr = get<LexList>(qq_arg.v.at(2));
    auto unq2 = get<LexSym>(unq2_expr.v.at(0));
    ASSERT_EQ(NAM_UNQUOTE, unq2.h);
    auto name2q = get<LexList>(unq2_expr.v.at(1));
    auto qt = get<LexSym>(name2q.v.at(0));
    ASSERT_EQ(NAM_QUOTE, qt.h);
    auto name2 = get<LexNam>(name2q.v.at(1));
    ASSERT_EQ(h + 3, name2.h);
    auto d = get<LexSym>(qq_arg.v.at(3));
    ASSERT_EQ(h + 4, d.h);
    auto e = get<LexSym>(t.v.at(2));
    ASSERT_EQ(h + 5, e.h);
}

