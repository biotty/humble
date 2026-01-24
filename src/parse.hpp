#ifndef HUMBLE_PARSE
#define HUMBLE_PARSE

#include "tok.hpp"
#include <memory>

namespace humble {

struct Macro {
    virtual Lex operator()(LexForm && t) = 0;
    bool is_user = false;  // instead of dynamic cast
    virtual ~Macro() = default;

    // note: certain macros are not to be copied in ops
    // such as import and macro, as each instance
    // will target a distinguished macro-set.  these
    // return nullptr so that macro-set clone does not
    // take a copy of these.  then added manually.
    virtual std::unique_ptr<Macro> clone() const = 0;
};

template <typename T>
struct MacroClone : Macro {
    std::unique_ptr<Macro> clone() const override {
        return std::make_unique<T>(static_cast<const T &>(* this));
    }
};

template <typename T>
struct MacroNotClone : Macro {
    std::unique_ptr<Macro> clone() const override { return nullptr; }
};

using Macros = std::map<int, std::unique_ptr<Macro>>;

Names init_names();  // known names by code-point
Macros qt_macros();  // macros needed as part of "parsing"
Macros clone_macros(Macros & macros);

LexForm readx(const std::string & s, Names & n);
LexForm parse(const std::string & s, Names & n, Macros & macros);
void expand_macros(Lex & t, Macros & macros, int qq);

struct Quote : MacroClone<Quote> { Lex operator()(LexForm && t); };
struct Quasiquote : MacroClone<Quasiquote> { Lex operator()(LexForm && t); };
struct Unquote : MacroClone<Unquote> { Lex operator()(LexForm && t); };

bool is_dotform(const LexForm & x);
LexForm without_dot(const LexForm & x);
LexForm with_dot(const LexForm & x);

Lex quote(Lex && t);

} // ns

#endif
