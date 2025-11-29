#include "api.hpp"
#include "utf.hpp"
#include "gtest/gtest.h"

using namespace humble;
using namespace std;

TEST(nth, ascii)
{
    string s = "a";
    ASSERT_EQ(s, utf_ref(s, 0).u);

    string t = "cab";
    ASSERT_EQ(s, utf_ref(t, 1).u);
}

TEST(nth, mb2)
{
    char a[] = { 99, (char)0b11000011, (char)0b10000000, 99 };
    string s{ a + 1, a + 3 };
    ASSERT_EQ(s, utf_ref(s, 0).u);

    string t{ a + 1, a + 4 };
    ASSERT_EQ(s, utf_ref(t, 0).u);

    string w{ a, a + 4 };
    ASSERT_EQ(s, utf_ref(w, 1).u);
    ASSERT_EQ("c", utf_ref(w, 2).u);
}

TEST(nth, mb3)
{
    char a[] = { 99, (char)0b11100010, (char)0b10000010,
        (char)0b10101100, 99 };
    string s{ a + 1, a + 4 };
    ASSERT_EQ(s, utf_ref(s, 0).u);

    string t{ a + 1, a + 5 };
    ASSERT_EQ(s, utf_ref(t, 0).u);

    string w{ a, a + 5 };
    ASSERT_EQ(s, utf_ref(w, 1).u);
    ASSERT_EQ("c", utf_ref(w, 2).u);
}

TEST(nth, mb4)
{
    char a[] = { 99, (char)0b11110000, (char)0b10011101,
        (char)0b10000101, (char)0b10100000, 99 };
    string s{ a + 1, a + 5 };
    ASSERT_EQ(s, utf_ref(s, 0).u);

    string t{ a + 1, a + 6 };
    ASSERT_EQ(s, utf_ref(t, 0).u);

    string w{ a, a + 6 };
    ASSERT_EQ(s, utf_ref(w, 1).u);
    ASSERT_EQ("c", utf_ref(w, 2).u);
}

TEST(dec, ascii)
{
    Glyph s{ "a" };
    ASSERT_EQ(97, utf_value(s));
}

TEST(dec, mb2)
{
    Glyph s{ "À" };
    ASSERT_EQ(192, utf_value(s));
}

TEST(dec, mb3)
{
    Glyph s{ "€" };
    ASSERT_EQ(8364, utf_value(s));
}

TEST(dec, mb4)
{
    Glyph s{ "𝅘𝅥𝅮" };
    ASSERT_EQ(119136, utf_value(s));
}

