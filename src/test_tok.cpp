#include "api.hpp"
#include "detail.hpp"
#include "tok.hpp"
#include "gtest/gtest.h"

using namespace humble;
using namespace std;

TEST(spaces, null_arg)
{
    ASSERT_THROW(spaces(nullptr, 0), CoreError);
}

TEST(spaces, empty)
{
    auto s = "";
    ASSERT_EQ(0, spaces(s, strlen(s)));
}

TEST(spaces, skip_one)
{
    auto s = " a";
    ASSERT_EQ(1, spaces(s, strlen(s)));
}

TEST(spaces, skip_some)
{
    linenumber = 0;
    auto s = "\t\n\r\v\ta";
    ASSERT_EQ(5, spaces(s, strlen(s)));
    EXPECT_EQ(1, linenumber);
}

TEST(spaces, skip_comment)
{
    linenumber = 0;
    auto s = " ;b\na";
    ASSERT_EQ(4, spaces(s, strlen(s)));
    EXPECT_EQ(1, linenumber);
}

TEST(spaces, skip_multiline_comment)
{
    linenumber = 0;
    auto s = " #|b\n\n|#a";
    ASSERT_EQ(8, spaces(s, strlen(s)));
    EXPECT_EQ(2, linenumber);
}

TEST(spaces, err_multiline_not_ended)
{
    auto s = "#|a";
    ASSERT_THROW(spaces(s, strlen(s)), SrcError);
}

TEST(tok, err_empty)
{
    ASSERT_THROW(tok(""), CoreError);
}

TEST(tok, skips_space)
{
    string s = " a";
    ASSERT_EQ(s.substr(1), tok(s).first);
}

TEST(tok, par)
{
    string s = "(a";
    ASSERT_EQ(s.substr(0, 1), tok(s).first);
}

TEST(tok, err_hash_end)
{
    ASSERT_THROW(tok("#"), SrcError);
    ASSERT_THROW(tok("#\\"), SrcError);
}

TEST(tok, hash_a)
{
    string s = "#aa/";
    ASSERT_EQ(s.substr(0, 3), tok(s).first);

    string b = "#\\aa/";
    ASSERT_EQ(b.substr(0, 4), tok(b).first);
}

TEST(tok, hash_nona)
{
    string s = "#/a";
    ASSERT_EQ(s.substr(0, 2), tok(s).first);

    string b = "#\\/a";
    ASSERT_EQ(b.substr(0, 3), tok(b).first);
}

TEST(tok, str_empty)
{
    string s = "\"\"";
    ASSERT_EQ(s, tok(s).first);
}

TEST(tok, str_nonempty)
{
    string s = "\"a\"";
    ASSERT_EQ(s, tok(s).first);
}

TEST(tok, str_dq)
{
    string s = "\"\\\"\"";
    ASSERT_EQ(s, tok(s).first);
}

TEST(tok, at_or_quote)
{
    string r = "@a";
    ASSERT_EQ(r.substr(0, 1), tok(r).first);

    string s = "'a";
    ASSERT_EQ(s.substr(0, 1), tok(s).first);

    string t = "`a";
    ASSERT_EQ(t.substr(0, 1), tok(t).first);

    string u = ",a";
    ASSERT_EQ(u.substr(0, 1), tok(u).first);
}

TEST(tok, name)
{
    string s = "a*";
    ASSERT_EQ(s, tok(s).first);

    string t = "*a*";
    ASSERT_EQ(t, tok(t).first);
}

TEST(unescape_string, special)
{
    string s = "\\t";
    ASSERT_EQ("\t", unescape_string(s));

    string t = "\\n";
    ASSERT_EQ("\n", unescape_string(t));

    string u = "\\r";
    ASSERT_EQ("\r", unescape_string(u));
}

TEST(unescape_string, octal)
{
    string s = "\\033";
    ASSERT_EQ("\33", unescape_string(s));

    string t = "\\33a";
    ASSERT_EQ("\33", unescape_string(t));
}

TEST(intern, nonexist)
{
    Names m = { "foo" };
    ASSERT_EQ(1, intern("bar", m));
    ASSERT_EQ(2, m.size());
}

TEST(intern, exist)
{
    Names m = { "foo", "bar", "duh" };
    ASSERT_EQ(1, intern("bar", m));
    ASSERT_EQ(3, m.size());
}

TEST(lex, shbang)
{
    Names m;
    ASSERT_THROW(lex("#!", m), SrcError);

    vector<Lex> v = lex("#!\n", m);
    ASSERT_EQ(0, v.size());
}

TEST(lex, par)
{
    Names m;

    vector<Lex> v = lex("(]{", m);
    ASSERT_EQ(3, v.size());
    ASSERT_EQ(0, get<LexBeg>(v[0]).par);
    ASSERT_EQ(1, get<LexEnd>(v[1]).par);
    ASSERT_EQ(2, get<LexBeg>(v[2]).par);
}

TEST(lex, par_line)
{
    Names m;
    linenumber = 0;

    vector<Lex> v = lex(")\n]\n\n}", m);
    ASSERT_EQ(3, v.size());
    ASSERT_EQ(0, get<LexEnd>(v[0]).line);
    ASSERT_EQ(1, get<LexEnd>(v[1]).line);
    ASSERT_EQ(3, get<LexEnd>(v[2]).line);
}

TEST(lex, num)
{
    Names m;

    vector<Lex> v = lex("110 #b110 #o110 #d110 #x110", m);
    ASSERT_EQ(5, v.size());
    EXPECT_EQ(110, get<LexNum>(v[0]).i);
    EXPECT_EQ(6, get<LexNum>(v[1]).i);
    EXPECT_EQ(72, get<LexNum>(v[2]).i);
    EXPECT_EQ(110, get<LexNum>(v[3]).i);
    EXPECT_EQ(272, get<LexNum>(v[4]).i);
}

TEST(lex, boolean)
{
    Names m;

    vector<Lex> v = lex("#f #t #false #true", m);
    ASSERT_EQ(4, v.size());
    ASSERT_EQ(false, get<LexBool>(v[0]).b);
    ASSERT_EQ(true, get<LexBool>(v[1]).b);
    ASSERT_EQ(false, get<LexBool>(v[2]).b);
    ASSERT_EQ(true, get<LexBool>(v[3]).b);
}

TEST(lex, chr)
{
    Names m;

    vector<Lex> v = lex("#\\a #\\alarm", m);
    ASSERT_EQ(2, v.size());
    ASSERT_EQ(97, get<LexNum>(v[0]).i);
    ASSERT_EQ(7, get<LexNum>(v[1]).i);
}

TEST(lex, chr_nonascii)
{
    Names m;

    vector<Lex> v = lex("#\\À #\\€ #\\𝅘𝅥𝅮", m);
    ASSERT_EQ(3, v.size());
    ASSERT_EQ(192, get<LexNum>(v[0]).i);
    ASSERT_EQ(8364, get<LexNum>(v[1]).i);
    ASSERT_EQ(119136, get<LexNum>(v[2]).i);
}

TEST(lex, str_adjacent)
{
    Names m;

    vector<Lex> v = lex("\"\"\"\"", m);
    ASSERT_EQ(2, v.size());
    ASSERT_EQ("", get<LexString>(v[0]).s);
    ASSERT_EQ("", get<LexString>(v[1]).s);
}

TEST(lex, str_nonascii)
{
    Names m;

    vector<Lex> v = lex("\"À\"", m);
    ASSERT_EQ(1, v.size());
    ASSERT_EQ("À", get<LexString>(v[0]).s);
}

TEST(lex, dot_quotes_void)
{
    Names m;

    vector<Lex> v = lex(".'`,#void", m);
    ASSERT_EQ(5, v.size());
    ASSERT_TRUE(std::holds_alternative<LexDot>(v[0]));
    ASSERT_TRUE(std::holds_alternative<LexQt>(v[1]));
    ASSERT_TRUE(std::holds_alternative<LexQqt>(v[2]));
    ASSERT_TRUE(std::holds_alternative<LexUnq>(v[3]));
    ASSERT_TRUE(std::holds_alternative<LexVoid>(v[4]));
}

TEST(lex, names)
{
    Names m;
    linenumber = 0;

    vector<Lex> v = lex("a b.b\nc@\n"
            "\n!$%&*+-./:<=>?@^_~", m);
    ASSERT_EQ(4, v.size());
    ASSERT_EQ(0, get<LexName>(v[0]).h);
    ASSERT_EQ(1, get<LexName>(v[1]).h);
    ASSERT_EQ(2, get<LexName>(v[2]).h);
    ASSERT_EQ(3, get<LexName>(v[3]).h);

    ASSERT_EQ(0, get<LexName>(v[0]).line);
    ASSERT_EQ(0, get<LexName>(v[1]).line);
    ASSERT_EQ(1, get<LexName>(v[2]).line);
    ASSERT_EQ(3, get<LexName>(v[3]).line);
}

TEST(lex, name_nonascii)
{
    Names m;

    ASSERT_THROW(lex("Àa", m), SrcError);
    ASSERT_THROW(lex("'Àa", m), SrcError);
}

