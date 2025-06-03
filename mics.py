#!/usr/bin/env python3

# tokenization

LEX_BEG        = 1001
LEX_END        = 1002
LEX_DOT        = 1003
LEX_SYM        = 1005
LEX_REF        = 1006
LEX_QT         = 1011
LEX_QQ         = 1012
LEX_UNQ        = 1013
LEX_UNQ_SPLICE = 1014

LEX_QT_BEG     = 1021  #| order respectively
LEX_QQ_BEG     = 1022
LEX_UNQ_BEG    = 1023
LEX_UNQ_SPLICE_BEG = 1024

name_cs = "!$%&*+-./:<=>?@^_~"
par_beg = "([{"  #| order respective to
par_end = ")]}"  #| closing
quotes  = "'`,"  #| order as their LEX_ codes

# syntax parse

LEX_VOID       = 1025
LEX_LIST       = 1027
LEX_NONLIST    = 1028
LEX_SPLICE     = 1029
LEX_NUM        = 1031
LEX_BOOL       = 1032
LEX_STRING     = 1033
LEX_NAM        = 1034

LEX_QUOTE      = 1041
LEX_QUASIQUOTE = 1042
LEX_UNQUOTE    = 1043
LEX_UNQUOTE_SPLICE = 1044

# runtime types

LEX_VAR_OFF    = 1000

VAR_VOID       = 2025  #| respecting LEX_VAR_OFF
VAR_CONS       = 2026
VAR_LIST       = 2027
VAR_NONLIST    = 2028
VAR_SPLICE     = 2029
VAR_NUM        = 2031
VAR_BOOL       = 2032
VAR_STRING     = 2033
VAR_NAM        = 2034
VAR_DICT       = 2035
VAR_REC        = 2036
VAR_QUOTE      = 2041  #| mostly unusable but we may happen to evaluate
VAR_UNQUOTE    = 2043  #| these things.
VAR_FUN        = 2061  #| result of lambda
VAR_FUN_DOT    = 2062  #|
VAR_APPLY      = 2063  #| defer call or inline tail-call optimization

# interpreter instructions

# OP_APPLY "form" implicit
OP_DEFINE      = 3001
OP_LAMBDA      = 3002
OP_LAMBDA_DOT  = 3003
OP_REBIND      = 3004
OP_COND        = 3005
OP_IMPORT      = 3006
OP_EXPORT      = 3007
OP_SEQ         = 3008

# known names

NAM_THEN       = 0  #| identifiers pre-interned as syntax but also
NAM_ELSE       = 1  #| to re-use for local names in a few macros
NAM_QUOTE      = 2      #| quote-processing are performed by macros
NAM_QUASIQUOTE = 3      #| on names
NAM_UNQUOTE    = 4      #|
NAM_UNQUOTE_SPLICE = 5  #| .
NAM_MACRO      = 6  #| the macro macro; defines a user-macro
NAM_CAR        = 7
NAM_EQVP       = 8
NAM_LENGTH     = 9
NAM_APPLY      = 10
NAM_LIST       = 11
NAM_NONLIST    = 12

nam_then = (LEX_NAM, NAM_THEN)
nam_else = (LEX_NAM, NAM_ELSE)
nam_quote = (LEX_NAM, NAM_QUOTE)
nam_quasiquote = (LEX_NAM, NAM_QUASIQUOTE)
nam_unquote = (LEX_NAM, NAM_UNQUOTE)
nam_unquote_splice = (LEX_NAM, NAM_UNQUOTE_SPLICE)
nam_macro = (LEX_NAM, NAM_MACRO)
nam_car = (LEX_NAM, NAM_CAR)
nam_eqvp = (LEX_NAM, NAM_EQVP)
nam_length = (LEX_NAM, NAM_LENGTH)
nam_apply = (LEX_NAM, NAM_APPLY)
nam_list = (LEX_NAM, NAM_LIST)
nam_nonlist = (LEX_NAM, NAM_NONLIST)

var_type_names = {
        VAR_VOID: "void", VAR_CONS: "cons", VAR_LIST: "list", VAR_NONLIST: "nonlist",
        VAR_SPLICE: "splice", VAR_NUM: "number", VAR_BOOL: "boolean",
        VAR_STRING: "string", VAR_DICT: "dict",
        VAR_NAM: "name", VAR_QUOTE: "quote", VAR_UNQUOTE: "unquote",
        VAR_FUN: "fun-obj", VAR_FUN_DOT: "dotfun-obj", VAR_APPLY: "to-apply" }

def lex_type_name(t):
    return var_type_names[t + LEX_VAR_OFF]

import sys
import itertools

def put(f, a):
    f.write(" ".join([str(x) for x in a]) + "\n")

verbose = False

def debug(*args):
    if verbose:
        put(sys.stdout, ["debug:"] + list(args))

def result(*args):
    put(sys.stdout, ["  ==>"] + list(args))

def warning(*args):
    put(sys.stdout, ["warning:"] + list(args))

def error(*args):
    put(sys.stderr, ["error:"] + list(args))
    sys.exit(1)

def broken(*args):
    put(sys.stderr, ["broken:"] + list(args))
    raise RuntimeError()

class SchemeSrcError(Exception):
    def __init__(self, message):
      super().__init__(message)

class SchemeRunError(Exception):
    def __init__(self, message):
      super().__init__(message)

filename = None
linenumber = None

def spaces(s, i, n):
    global linenumber
    while i != n:
        if s[i].isspace():
            if s[i] == '\n':
                linenumber += 1
            i += 1
        elif s[i] == ';':
            # comment
            while s[i] != '\n':
                i += 1
        elif s[i] == '#':
            if i + 1 != n and s[i + 1] == '|':
                # multiline-comment
                i += 2
                x = None
                while i != n:
                    y = s[i]
                    if y == '\n':
                        linenumber += 1
                    if x == '|' and y == '#':
                        i += 1
                        break
                    x = y
                    i += 1
                else:
                    raise SchemeSrcError("#| comment not ended")
            else:
                break
        else:
            break
    return i

def tok(s, i):
    n = len(s)
    i = spaces(s, i, n)
    if i == n:
        return None, i
    w = i
    if s[i] in (par_beg + par_end):
        return s[i], i + 1
    if s[i] == "#":
        i += 1
        if i == n:
            raise SchemeSrcError("terminated at #")
        if s[i] == "\\":
            i += 1
        if not s[i].isalnum():
            i += 1
            if i == n:
                raise SchemeSrcError("terminated at #\\")
        else:
            while i < n and s[i].isalnum():
                i += 1
        return s[w : i], i
    if s[i] == "\"":
        while i < n:
            i += 1
            if i == n:
                raise SchemeSrcError("terminated in string")
            if s[i] == "\"":
                i += 1
                return s[w : i], i
            if s[i] == "\\":
                i += 1
                if i == n:
                    raise SchemeSrcError("terminated in string at '\\'")
    if s[i] in quotes:
        i += 1
        if i == n:
            raise SchemeSrcError("terminated at quote")
        if s[i].isspace() or s[i].isdecimal():
            return s[w : i], i
        if s[i] == "@":
            i += 1
            if i == n:
                raise SchemeSrcError("terminated at @")
        if s[i] in par_beg:
            i += 1
            return s[w : i], i
    while s[i].isalnum() or s[i] in name_cs:
        i += 1
        if i == n:
            break
    if i != w:
        return s[w : i], i
    raise SchemeSrcError("character '%c' on line %d" %(s[i], linenumber))

def unescape_string(s):
    a = []
    i = 0
    n = len(s)
    while i < n:
        c = s[i]
        if c == "\\":
            i += 1
            if i == n:
                raise SchemeSrcError("at %d in string on line %d" % (i, linenumber))
            c = s[i]
            if c == "t":
                c = "\t"
            if c == "n":
                c = "\n"
            if c == "r":
                c = "\r"
        a.append(c)
        i += 1
    return "".join(a)

# note:  tokens considered names are converted to integers early,
#        with a name list as a bi-product, with respective names.

def intern(name, names):
    try:
        return names.index(name)
    except:
        i = len(names)
        names.append(name)
        return i

def name_tok(t, names):
    y = LEX_NAM
    h = None
    if t[0] in quotes:
        y = quotes.index(t[0]) + LEX_QT
        if len(t) == 1:
            h = -1
        elif t[1] in par_beg:
            y += LEX_QT_BEG - LEX_QT
            h = par_beg.index(t[1])
        elif t.startswith(",@"):
            if len(t) == 2:
                h = -1
            elif t[2] in par_beg:
                y = LEX_UNQ_SPLICE_BEG
                h = par_beg.index(t[2])
            else:
                y = LEX_UNQ_SPLICE
                t = t[2:]
        else:
            t = t[1:]
            if y != LEX_UNQ:
                y = LEX_SYM
    if h is not None:
        v = (y, h)
    else:
        h = intern(t, names)
        v = (y, h, linenumber)
    debug("ntok", v)
    return v

def lex(s, names):
    n = len(s)
    i = 0
    a = []
    while i != n:
        t, i = tok(s, i)
        if not t:
            break
        v = None
        if t in par_beg:
            v = (LEX_BEG, par_beg.index(t))
        elif t in par_end:
            v = (LEX_END, par_end.index(t), linenumber)
        elif (t[0] in "-+." and len(t) != 1) or t[0].isdigit():
            v = (LEX_NUM, int(t))
        elif t[0] == "#":
            if t[1] in "tf" and len(t) == 2:
                v = (LEX_BOOL, "t" == t[1])
            elif t[1] == "\\":
                if len(t) == 3:
                    v = (LEX_NUM, ord(t[2]))
                elif t[2:] == "tab":
                    v = (LEX_NUM, 9)
                elif t[2:] == "newline":
                    v = (LEX_NUM, 10)
                elif t[2:] == "return":
                    v = (LEX_NUM, 13)
                elif t[2:] == "space":
                    v = (LEX_NUM, 32)
                else:
                    raise SchemeSrcError("#\ token at line %d" % (linenumber,))
            else:
                raise SchemeSrcError("# token at line %d" % (linenumber,))
        elif t[0] == "\"":
            v = (LEX_STRING, unescape_string(t[1:-1]))
        elif t == ".":
            v = (LEX_DOT, None)
        else:
            v = name_tok(t, names)
        a.append(v)
    debug("lex", a)
    return a

PARSE_MODE_TOP = -1
PARSE_MODE_ONE = -2
def parse_r(z, i, paren_mode, d):
    n = len(z)
    r = []
    while i != n:
        x = z[i]
        c = x[0]
        if c == LEX_END:
            if x[1] != paren_mode:
                raise SchemeSrcError("parens '%s' at line %d does not match '%s'" % (
                    par_beg[paren_mode] if paren_mode >= 0 else '(none)',
                    x[2], par_end[x[1]]))
            return r, i + 1
        elif c in (LEX_BEG, LEX_QT_BEG, LEX_QQ_BEG, LEX_UNQ_BEG, LEX_UNQ_SPLICE_BEG):
            s, i = parse_r(z, i + 1, x[1], d + 1)
            if c == LEX_QT_BEG:
                r.append([(LEX_NAM, NAM_QUOTE), s])
            elif c == LEX_QQ_BEG:
                r.append([(LEX_NAM, NAM_QUASIQUOTE), s])
            elif c == LEX_UNQ_BEG:
                r.append([(LEX_NAM, NAM_UNQUOTE), s])
            elif c == LEX_UNQ_SPLICE_BEG:
                r.append([(LEX_NAM, NAM_UNQUOTE_SPLICE), s])
            elif c == LEX_BEG:
                r.append(s)
            else:
                broken("%d unexpected" % (c,))
        elif x[1] == -1 and c in (LEX_SYM, LEX_QT, LEX_QQ, LEX_UNQ, LEX_UNQ_SPLICE):
            x, i = parse_r(z, i + 1, PARSE_MODE_ONE, d)
            if len(x) != 1:
                broken("expected 1 token in one-mode")
            if c == LEX_SYM:
                x = [(LEX_NAM, NAM_QUOTE), *x]
            elif c == LEX_QT:
                x = [(LEX_NAM, NAM_QUOTE), *x]
            elif c == LEX_QQ:
                x = [(LEX_NAM, NAM_QUASIQUOTE), *x]
            elif c == LEX_UNQ:
                x = [(LEX_NAM, NAM_UNQUOTE), *x]
            elif c == LEX_UNQ_SPLICE:
                x = [(LEX_NAM, NAM_UNQUOTE_SPLICE), *x]
            else:
                broken("%d unexpected" % (c,))
            r.append(x)
        else:
            if c == LEX_SYM:
                x = [(LEX_NAM, NAM_QUOTE), (LEX_NAM, x[1])]
            elif c == LEX_UNQ:
                x = [(LEX_NAM, NAM_UNQUOTE), (LEX_NAM, x[1])]
            elif c == LEX_UNQ_SPLICE:
                x = [(LEX_NAM, NAM_UNQUOTE), (LEX_SPLICE, (LEX_NAM, x[1]))]
            i += 1
            r.append(x)
        if paren_mode == PARSE_MODE_ONE:
            debug("parse1", r)
            return r, i
    if paren_mode != PARSE_MODE_TOP:
        raise SchemeSrcError("parens '%s' depth %d not closed" % (
            par_beg[paren_mode], d))
    debug("parse", r)
    return r, i

def is_dotform(x):
    if len(x) < 2:
        return False
    e = x[-2]
    return type(e) != list and e[0] == LEX_DOT

def without_dot(x):
    assert x[-2][0] == LEX_DOT
    return x[:-2] + x[-1:]

def with_dot(x):
    assert x[-2][0] != LEX_DOT
    return x[:-1] + [(LEX_DOT, None), x[-1]]

def expand_macros(t, macros, qq):
    if not (type(t) == list and t):
        return t
    is_macro = t[0] and t[0][0] == LEX_NAM and t[0][1] in macros
    is_user = is_macro and hasattr(macros[t[0][1]], "user")
    current = is_macro and qq == 0
    is_quote = False
    if is_macro:
        if t[0][1] == NAM_QUOTE:
            is_quote = True
        elif t[0][1] == NAM_QUASIQUOTE:
            qq += 1
        elif t[0][1] in (NAM_UNQUOTE, NAM_UNQUOTE_SPLICE):
            qq -= 1
            if qq == 0:
                current = True
    d = macros
    if is_user:
        d = {
                NAM_QUOTE: macros[NAM_QUOTE],
                NAM_QUASIQUOTE: macros[NAM_QUASIQUOTE],
                NAM_UNQUOTE: macros[NAM_UNQUOTE],
                NAM_UNQUOTE_SPLICE: macros[NAM_UNQUOTE_SPLICE] }
    elif is_quote:
        d = {
                NAM_QUASIQUOTE: macros[NAM_QUASIQUOTE],
                NAM_UNQUOTE: macros[NAM_UNQUOTE],
                NAM_UNQUOTE_SPLICE: macros[NAM_UNQUOTE_SPLICE] }
    for i in range(len(t)):
        t[i] = expand_macros(t[i], d, qq)
    if current:
        t = macros[t[0][1]](t)
        if is_user and type(t) == list:
            t = expand_macros(t, macros, qq)
    debug("expand", t)
    return t

def parse_i(s, names, macros):
    z = lex(s, names)
    ast, i = parse_r(z, 0, PARSE_MODE_TOP, 0)
    if i != len(z):
        broken("no exception but not fully consumed; unexpected")
    try:
        expand_macros(ast, macros, 0)
    except KeyError as e:
        i = e.args[0]
        raise SchemeSrcError("%s unbound in macro-expand" % (names[i],))
    except IndexError as e:
        raise SchemeSrcError("form syntax error")
    debug("tree", ast)
    return ast

# names needed to be captured by a lambda are collected
# lexically, so that when instantiating a function
# when running, they are captured from the environment.

def unbound(s, defs, is_block):
    r = set()
    from_branches = set()
    for x in s:
        if type(x) != list:
            if x[0] == LEX_NAM and x[1] not in defs:
                r.add(x[1])
            if x[0] in (LEX_LIST, LEX_NONLIST):
                r.update(unbound(x[1], defs, False))
            if x[0] == LEX_SPLICE:
                r.update(unbound([x[1]], defs, False))
            continue
        if x[0] == OP_DEFINE:
            if not is_block:
                raise SchemeSrcError("define in non-block")
            r.update(unbound([x[2]], defs, False))
            defs.add(x[1])
            continue
        if x[0] in (OP_LAMBDA, OP_LAMBDA_DOT):
            from_branches.update(x[2])
            continue
        if x[0] == OP_COND:
            r.update(unbound(x[1:], defs, False))
            continue
        if x[0] == OP_IMPORT:
            defs.update(x[1])
            # note: no recurse, as checked when generating it
            continue
        if x[0] == OP_EXPORT:
            # note: ignored by itself.  in import handled above
            continue
        if x[0] == OP_SEQ:
            for y in x[1:]:
                r.update(unbound([y], defs, True))
            continue

        r.update(unbound(x, defs, False))
    from_branches.difference_update(defs)
    r.update(from_branches)
    return r

def locate_unbound(s, y):
    for x in s:
        if type(x) != list:
            if x[0] == LEX_NAM and x[1] == y:
                return x
            r = None
            if x[0] in (LEX_LIST, LEX_NONLIST):
                r = locate_unbound(x[1], y)
            if x[0] == LEX_SPLICE:
                r = locate_unbound([x[1]], y)
            if r:
                return r
            continue
        if x[0] == OP_DEFINE:
            r = locate_unbound([x[2]], y)
            if r:
                return r
            continue
        if x[0] in (OP_LAMBDA, OP_LAMBDA_DOT):
            if y in x[2]:
                return locate_unbound(x[3:], y)
            continue
        if x[0] == OP_COND:
            r = locate_unbound(x[1:], y)
            if r:
                return r
            continue
        if x[0] in (OP_IMPORT, OP_EXPORT):
            continue
        if x[0] == OP_REBIND:
            if y in x[1]:
                return x
            continue
        if x[0] == OP_SEQ:
            r = locate_unbound(x[1:], y)
            if r:
                return r
            continue
        r = locate_unbound(x, y)
        if r:
            return r

def info_unbound(x, names):
    a = []
    if x[0] != LEX_NAM:
        a.append("(non-name)")
    elif len(x) == 3:
        a.append("line %d: " % (x[2]))
    a.append("%s" % (names[x[1]],))
    return "".join(a)

def parse(s, names, macros, env_keys):
    global linenumber
    linenumber = 1
    t = parse_i(s, names, macros)
    u = unbound(t, set(env_keys), True)
    if not u:
        return t
    a = []
    for y in u:
        x = locate_unbound(t, y)
        if x:
            a.append(info_unbound(x, names))
        else:
            a.append("(reportedly) %s" % (names[y],))
    raise SchemeSrcError("unbound:\n" + "\n".join(a))

#
# macros
#

def mchk_or_fail(c, message):
    if not c:
        raise SchemeSrcError(message)

def margc_must_chk(c, mn, x, z):
    mchk_or_fail(c, "%s expects %d args but got %d"
            % (mn, z - 1, x - 1))

def margc_must_eq(mn, s, z):
    margc_must_chk(len(s) == z, mn, len(s), z)

def margc_must_ge(mn, s, z):
    margc_must_chk(len(s) >= z, mn, len(s), z)

def m_seq(s):
    s[0] = OP_SEQ
    return s

def m_define(s):
    s[0] = OP_DEFINE
    if type(s[1]) == list:
        n = s[1][0]
        s[2] = m_lambda([-99, s[1][1:], *s[2:]])
        s[1] = n
        s = m_define(s)
    else:
        mchk_or_fail(s[1][0] == LEX_NAM, "define.1 expects name")
        s[1] = s[1][1]
    i = s[1]
    d = s[2]
    u = unbound([d], set(), False)
    if i in u:
        y = (LEX_NAM, i)
        return [OP_DEFINE, i,
                m_letrec([-99, [[y, d]], y])]
    return s

def m_lambda(s):
    margc_must_ge("lambda", s, 3)
    s[0] = OP_LAMBDA
    y = s[1]
    if type(y) != list:
        mchk_or_fail(y[0] == LEX_NAM, "lambda.1 expected a name")
        s[0] = OP_LAMBDA_DOT
        y = [y]
    elif is_dotform(y):
        y = without_dot(y)
        s[0] = OP_LAMBDA_DOT
    a = []
    for x in y:
        mchk_or_fail(x[0] == LEX_NAM, "lambda params must be names")
        a.append(x[1])
    s[1] = a
    s.insert(2, unbound(s[2:], set(a), True))
    return s

def m_case_lambda(s):
    return m_lambda([-99, nam_then,
        [nam_apply, m_case([-99, [nam_length, nam_then],
            *[[[(LEX_NUM, len(c[0]))],
                m_lambda([-99, *c])] for c in s[1:]]]), nam_then]])

def bnd_unzip(s):
    if type(s) != list:
        broken("not list")
    a = []
    v = []
    for z in s:
        if type(z) != list:
            broken("not list")
        x, y = z
        if x[0] != LEX_NAM:
            broken("not name")
        a.append(x[1])
        v.append(y)
    return a, v

def m_let(s):
    margc_must_ge("let", s, 2)
    if type(s[1]) != list and s[1][0] == LEX_NAM:
        return named_let(s)
    lbd = [OP_LAMBDA]
    a, v = bnd_unzip(s[1])
    lbd.append(a)
    lbd.append(unbound(s[2:], set(a), True))
    lbd.extend(s[2:])
    return [lbd, *v]

def m_letx(s):
    margc_must_ge("let*", s, 2)
    block = s[2:]
    if not block:
        block = [(LEX_VOID,)]
    mchk_or_fail(type(s[1]) == list, "let*.1 expected sub-form")
    if not s[1]:
        return [[OP_LAMBDA, [], unbound(block, {}, True), *block]]
    for z in reversed(s[1]):
        lbd = [OP_LAMBDA]
        x, y = z
        mchk_or_fail(x[0] == LEX_NAM, "let* expects name in form")
        a = [x[1]]
        lbd.append(a)
        lbd.append(unbound(block, set(a), True))
        lbd.extend(block)
        block = [[lbd, y]]
    return block[0]

def rec(let):
    a = let[0][1]
    u = let[0][2]
    let[0].insert(3, [OP_REBIND, a])
    v = []
    for z in a:
        v.append((LEX_REF, z))
    u.update(unbound(let[1:], a, False))
    return [[OP_LAMBDA, a, u, let], *v]

def m_letrec(s):
    return rec(m_let(s))

def m_letrecx(s):
    return rec(m_letx(s))

def named_let(s):
    name = s[1]
    a, v = bnd_unzip(s[2])
    block = s[3:]
    u = unbound(block, set(a), True)
    return m_letrec([-99,
        [[name, [OP_LAMBDA, a, u, *block]]],
        [name, *v]])

def m_do(s):
    margc_must_ge("do", s, 3)
    step = []
    parms = s[1]
    for x in parms:
        if len(x) == 3:
            y = x.pop()
        else:
            mchk_or_fail(len(x) == 2, "do sub-form size 2 expected")
            y = x[0]
        step.append(y)
    s.append([nam_else, *step])
    return named_let([-99, nam_else, parms, m_if([-99, s[2][0],
        m_begin([-99, *s[2][1:]]),
        m_begin([-99, *s[3:]])])])

def m_begin(s):
    return m_letx([-99, [], *s[1:]])

def quote(v, quasi):
    if type(v) == list:
        if is_dotform(v):
            w = [quote(x, quasi) for x in without_dot(v)]
            if w[-1][0] in (LEX_LIST, LEX_NONLIST):
                z = w.pop()
                w.extend(z[1])
                return (z[0], w)
            return (LEX_NONLIST, w)
        return (LEX_LIST, [quote(x, quasi) for x in v])
    if v[0] == LEX_NAM:
        return (LEX_SYM, v[1])
    if v[0] in (LEX_SYM, LEX_LIST, LEX_NONLIST, LEX_QUOTE):
        return (LEX_QUOTE + int(quasi), v)
    if quasi:
        if v[0] == LEX_UNQ:
            return (LEX_NAM, v[1])
        if v[0] == LEX_UNQ_SPLICE:
            return (LEX_SPLICE, (LEX_NAM, v[1]))
        if v[0] == LEX_UNQUOTE:
            return v[1]
        if v[0] == LEX_UNQUOTE_SPLICE:
            return (LEX_SPLICE, v[1])
    if v[0] not in (LEX_NUM, LEX_BOOL, LEX_STRING, LEX_SPLICE):
        raise SchemeSrcError("quote of %s" % (lex_type_name(v[0]),))
    return v

def m_quote(s):
    margc_must_eq("quote", s, 2)
    return quote(s[1], False)

def m_quasiquote(s):
    margc_must_eq("quasiquote", s, 2)
    return quote(s[1], True)

def m_unquote(s):
    margc_must_eq("unquote", s, 2)
    def unquote(v):
        if type(v) == list:
            return (LEX_UNQUOTE, v)
        if v[0] == LEX_LIST:
            return v[1]
        if v[0] == LEX_NONLIST:
            return with_dot(v[1])
        if v[0] in (LEX_UNQ, LEX_NAM, LEX_UNQUOTE_SPLICE, LEX_UNQUOTE):
            return (LEX_UNQUOTE, v)
        if v[0] == LEX_SYM:
            return (LEX_NAM, v[1])
        if v[0] not in (LEX_NUM, LEX_BOOL, LEX_STRING, LEX_SPLICE):
            raise SchemeSrcError("unquote of %s" % (lex_type_name(v[0]),))
        # note:  no (un)quote-level for literals)
        return v
    return unquote(s[1])

def m_unquote_splice(s):
    margc_must_eq("unquote-splice", s, 2)
    return (LEX_UNQUOTE_SPLICE, s[1])

def from_lex(s):
    if type(s) == list:
        if is_dotform(s):
            return [VAR_NONLIST,
                    [from_lex(x) for x in without_dot(s)]]
        return [VAR_LIST, [from_lex(x) for x in s]]
    if s[0] == LEX_LIST:
        return from_lex([nam_list, *s[1]])
    if s[0] == LEX_NONLIST:
        return from_lex([nam_nonlist, *s[1]])
    if s[0] == LEX_SYM:
        return from_lex([(LEX_NAM, NAM_QUOTE), (LEX_NAM, s[1])])
    if s[0] not in (LEX_NAM, LEX_NUM, LEX_BOOL, LEX_STRING,
            LEX_QUOTE, LEX_UNQUOTE):
        raise SchemeSrcError("from lex <%d>" % (s[0],))
    return [s[0] + LEX_VAR_OFF, s[1]]

def to_lex(s):
    if s[0] == VAR_CONS:
        s = s[1].to_list_var()
    if s[0] in (VAR_LIST, VAR_NONLIST):
        r = [to_lex(x) for x in s[1]]
        return with_dot(r) if s[0] == VAR_NONLIST else r
    if s[0] == VAR_SPLICE:
        return (LEX_SPLICE, [to_lex(x) for x in s[1]])
    if s[0] == VAR_VOID:
        return (LEX_VOID,)
    if s[0] not in (VAR_NAM, VAR_NUM, VAR_BOOL, VAR_STRING,
            VAR_QUOTE, VAR_UNQUOTE):
        raise SchemeSrcError("to lex <%d>" % (s[0],))
    return (s[0] - LEX_VAR_OFF, s[1])

def m_macro(macros):
    def macro(s):
        margc_must_ge("macro", s, 4)
        if s[1][0] != LEX_NAM:
            raise SchemeSrcError("non-name macro")
        n = s[1][1]
        y = s[2]
        if type(y) != list:
            dot = True
            y = [y]
        else:
            dot = is_dotform(y)
            if dot:
                y = without_dot(y)
        block = s[3:]
        parms = []
        for p in y:
            mchk_or_fail(p[0] == LEX_NAM, "macro params must be names")
            parms.append(p[1])
        def m(t):
            args = [from_lex(x) for x in t[1:]]
            debug("user-macro args", args)
            env = ExtraDict(i_env)
            if dot:
                last = len(parms) - 1
                mchk_or_fail(len(args) >= last, "macro"
                        " (dot) invoked with few args")
                for i in range(last):
                    env[parms[i]] = args[i]
                env[parms[last]] = [VAR_LIST, args[last:]]
            else:
                mchk_or_fail(len(args) == len(parms), "macro"
                        " invoked with bad arg-count")
                for i, v in enumerate(args):
                    env[parms[i]] = v
            for x in block:
                r = run(x, env)
            debug("user-macro result", r)
            r = to_lex(r)
            debug("result as lex", r)
            return r
        m.user = True
        macros[n] = m
        return (LEX_VOID,)
    return macro

def m_gensym(names):
    def gensym(s):
        i = len(names)
        names.append("&%d&" % (i,))
        return (LEX_SYM, i)
    return gensym

# bare lex, skipping linenumber info
# eases comparison
def blex(y):
    return y[:2]

def m_cond(s):
    s[0] = OP_COND
    if len(s) == 1:
        return s
    n = len(s)
    i = 1
    while i < n:
        if blex(s[i][0]) == nam_else:
            return s[:i] + [[(LEX_BOOL, True),
                m_begin([-99, *s[i][1:]])]]
        if len(s[i]) != 2:
            break
        i += 1
    if i == n:
        return s
    assert blex(s[i][1]) == nam_then
    return s[:i] + [[(LEX_BOOL, True),
        m_let([-99, [[nam_then, s[i][0]]],
            m_cond([-99, [nam_then, [s[i][2], nam_then]], *s[i + 1:]])])]]

def m_if(s):
    if len(s) == 3:
        s.append([(LEX_BOOL, True), (LEX_VOID,)])
    elif len(s) != 4:
        raise SchemeSrcError("if-expression of length %d"
                % (len(s),))
    return [OP_COND, [s[1], s[2]], [(LEX_BOOL, True), s[3]]]

def and_r(s):
    if len(s) == 2:
        return s[1]
    return [OP_COND, [s[1], and_r([-99, *s[2:]])],
            [(LEX_BOOL, True), (LEX_BOOL, False)]]

def m_and(s):
    if len(s) == 1:
        return (LEX_BOOL, True)
    return and_r(s)

def m_or(s):
    if len(s) == 1:
        return (LEX_BOOL, False)
    return m_let([-99, [[nam_then, s[1]]],
        [OP_COND, [nam_then, nam_then],
            [(LEX_BOOL, True), m_or([-99, *s[2:]])]]])

def m_when(s):
    margc_must_ge("when", s, 2)
    return [OP_COND, [s[1], m_begin([-99, *s[2:]])],
            [(LEX_BOOL, True), (LEX_VOID,)]]

def m_unless(s):
    margc_must_ge("unless", s, 2)
    return [OP_COND, [s[1], (LEX_VOID,)],
            [(LEX_BOOL, True), m_begin([-99, *s[2:]])]]

def xcase_test(t, last):
    if type(t) != list:
        assert last
        assert blex(t) == nam_else
        return (LEX_BOOL, True)
    return m_or([-99, *[[nam_eqvp, v, nam_else] for v in t]])

def xcase_target(t):
    if blex(t[0]) == nam_then:
        assert len(t) == 2
        t = [[t[1], nam_else]]
    return (LEX_LIST, t)

def xcase(s):
    m = len(s) - 1
    return m_or([-99, *[
        m_and([-99, xcase_test(ce[0], i == m),
            xcase_target(ce[1:])
            if blex(ce[0]) != nam_else or blex(ce[1]) == nam_then
            else (LEX_LIST, [m_begin([-99, *ce[1:]])])])
        for i, ce in enumerate(s)]])

def m_case(s):
    return [nam_car, m_letx([-99, [[nam_else, s[1]]],
        xcase(s[2:])])]

def m_export(s):
    s[0] = OP_EXPORT
    return s

class ExtraDict:

    def __init__(self, d):
        self.d = d
        self.e = dict()

    def __contains__(self, k):
        return k in self.e or k in self.d

    def __getitem__(self, k):
        if k in self.e:
            return self.e[k]
        return self.d[k]

    def __setitem__(self, k, v):
        self.e[k] = v

def m_scope(s):
    # a.k.a "here-import"
    set_up = dict()
    if s[1][0] != OP_EXPORT:
        raise SchemeSrcError("missing export")
    for n in s[1][1:]:
        if n[0] != LEX_NAM:
            raise SchemeSrcError("export of non-name")
        y = n[1]
        set_up[y] = y
    return [OP_IMPORT, set_up, *s[2:]]

def m_import(names, macros):
    def ximport(s):
        global filename
        u_fn = filename
        mchk_or_fail(len(s) in (2, 3), "import expects 2 or 3 args")
        mchk_or_fail(s[1][0] == LEX_STRING, "import.1 expects string")
        filename = s[1][1]
        e_macros = ExtraDict(macros)
        e_macros[NAM_MACRO] = m_macro(e_macros)
        try:
            f = open(filename, "r")
        except:
            raise SchemeSrcError("no such file")
        global linenumber
        u_ln = linenumber
        r = parse(f.read(), names, e_macros, i_env.keys())
        f.close()
        prefix_s = None
        if len(s) == 3:
            p = s[2]
            if p[0] not in (LEX_NAM, LEX_SYM):
                raise SchemeSrcError("non-name import-prefix")
            prefix_s = names[p[1]]
        set_up = dict()
        if r[0][0] != OP_EXPORT:
            raise SchemeSrcError("missing export")
        for n in r[0][1:]:
            x = y = n[1]
            if n[0] == LEX_NAM:
                if prefix_s:
                    y = intern(prefix_s + names[x], names)
                set_up[y] = x
            elif n[0] == LEX_SYM:
                if prefix_s and p[0] == LEX_SYM:
                    y = intern(prefix_s + names[x], names)
                if x not in e_macros:
                    raise SchemeSrcError("no macro %s"
                            % (names[x],))
                macros[y] = e_macros[x]
            else:
                raise SchemeSrcError("export of non-name")
        # note:  unbound check with i_env not needed, as this will
        #        be done from top-level
        filename = u_fn
        linenumber = u_ln
        return [OP_IMPORT, set_up, *r[1:]]
    return ximport

# a list variable is represented by a contiguous container
# until cons-usage requires it to convert to a cons.
# use *list-copy* to establish contigous array.
#
# for the contiguous container, the type denotes if it represent
# a list, or that it represents a cons-chain where the last
# CDR is not NIL, by the types VAR_LIST and VAR_NONLIST.

def is_cons(y):
    return isinstance(y, Cons)

class Cons:

    # note: non-recursive impls

    def __init__(self, a, d):
        self.a = a  # CAR will be a VAR_xyz
        self.d = d  # CDR will be a Cons (not VAR_CONS) or None,
        #             or in case of a non-list will be a VAR_xyz

    def xcopy(self, n):
        r = c = Cons(self.a, self.d)
        while c.d is not None:
            assert is_cons(c.d)
            c.d = Cons(c.d.a, c.d.d)
            c = c.d
            n -= 1
            if n == 0:
                break
        else:
            Cons.last_cons = c
        return r

    def to_list_var(self):
        t = VAR_LIST
        r = []
        c = self
        while c is not None:
            if not is_cons(c):
                r.append(c)
                t = VAR_NONLIST
                break
            r.append(c.a)
            c = c.d
        return [t, r]

    def length(self):
        r = 1
        c = self
        while c.d is not None:
            assert is_cons(c.d)
            r += 1
            c = c.d
        return r

    @staticmethod
    def from_list(x):
        if len(x) == 0:
            return None
        r = Cons(x[-1], None)
        Cons.last_cons = r
        for e in reversed(x[:-1]):
            r = Cons(e, r)
        return r

    @staticmethod
    def from_nonlist(x):
        assert len(x) >= 2
        Cons.last_cons = None
        r = x[-1]
        for e in reversed(x[:-1]):
            r = Cons(e, r)
        return r

def to_cons(a):
    if a[0] == VAR_CONS:
        return a[1]
    if a[0] == VAR_LIST:
        return Cons.from_list(a[1])
    if a[0] == VAR_NONLIST:
        return Cons.from_nonlist(a[1])

def to_cons_copy(a):
    if a[0] == VAR_CONS:
        return a[1].xcopy(0)
    return to_cons(a)

def normal_list(a):
    if is_cons(a):
        r = a.to_list_var()
        if r[0] != VAR_LIST:
            raise SchemeRunError("nonlist for list-use")
        return r[1]
    if a is None:
        return []
    assert type(a) == list
    return a

#
# builtin functions
#

def fargt_repr(vt):
    try:
        return var_type_names[vt]
    except KeyError:
        return "[%d]" % (vt,)

def fchk_or_fail(c, message):
    if not c:
        raise SchemeRunError(message)

def fargs_count_fail(fn, k, n):
    raise SchemeRunError("%s expects %d args but got %d"
            % (fn, n, k))

def fargc_must_eq(fn, args, n):
    if len(args) != n:
        fargs_count_fail(fn, len(args), n)

def fargc_must_ge(fn, args, n):
    if len(args) < n:
        fargs_count_fail(fn, len(args), n)

def fargt_must_eq(fn, args, i, vt):
    fargc_must_ge(fn, args, i + 1)
    if args[i][0] != vt:
        raise SchemeRunError("%s expected args[%d] %s got %s"
            % (fn, i,
                fargt_repr(vt), fargt_repr(args[i][0])))

def fargt_must_in(fn, args, i, vts):
    fargc_must_ge(fn, args, i + 1)
    if args[i][0] not in vts:
        raise SchemeRunError("%s args[%d] accepts %r got %s"
            % (fn, i, "/".join(fargt_repr(vt) for vt in vts),
                fargt_repr(args[i][0])))

def f_list(*args):
    return [VAR_LIST, list(args)]

def f_nonlist(*args):
    # note: not cat'ing as quote does on (nested) dots
    return [VAR_NONLIST, list(args)]

def f_list_copy(*args):
    fargt_must_in("list-copy", args, 0, (VAR_LIST, VAR_CONS))
    return [VAR_LIST, normal_list(args[0][1])]

def f_cons(*args):
    fargc_must_eq("cons", args, 2)
    if args[1][0] in (VAR_CONS, VAR_LIST, VAR_NONLIST):
        c = to_cons(args[1])
        args[1][:] = [VAR_CONS, c]
        return [VAR_CONS, Cons(args[0], c)]
    return [VAR_NONLIST, list(args)]

def f_car(*args):
    fargt_must_in("car", args, 0, (VAR_CONS, VAR_LIST, VAR_NONLIST))
    if args[0][0] in (VAR_LIST, VAR_NONLIST):
        return args[0][1][0]
    assert args[0][0] == VAR_CONS
    assert is_cons(args[0][1])
    return args[0][1].a

def f_list_ref(*args):
    def c_list_ref(c, i):
        if i == 0:
            return c.a
        return c_list_ref(c.d, i - 1)
    fargt_must_eq("list-ref", args, 1, VAR_NUM)
    if args[0][0] == VAR_CONS:
        return c_list_ref(args[0][1], args[1][1])
    assert args[0][0] in (VAR_LIST, VAR_NONLIST)
    return args[0][1][args[1][1]]

def f_cdr(*args):
    fargt_must_in("cdr", args, 0, (VAR_CONS, VAR_LIST, VAR_NONLIST))
    t = args[0][0]
    a = args[0][1]
    if t != VAR_CONS:
        assert t in (VAR_LIST, VAR_NONLIST)
        if t == VAR_NONLIST:
            assert len(a) > 1
            if len(a) == 2:
                return a[1]
        a = to_cons([t, a])
        args[0][:] = [VAR_CONS, a]
    if a is None:
        raise SchemeRunError("cdr on null")
    return [VAR_CONS, a.d]

def f_append(*args):
    if len(args) == 0:
        return [VAR_LIST, []]
    if len(args) == 1:
        return args[0]
    i_last = len(args) - 1
    last = args[i_last]
    if last[0] in (VAR_LIST, VAR_NONLIST):
        last[1] = to_cons(last)
        last[0] = VAR_CONS
    r = p = to_cons_copy(args[0])
    q = None
    for i, c in enumerate(args):
        if i == 0:
            continue
        if p is not None:
            q = Cons.last_cons
        if i == i_last:
            if last[0] == VAR_CONS:
                p = last[1]
            else:
                p = last
        else:
            fargt_must_in("append", args, i, (VAR_CONS, VAR_LIST))
            p = to_cons_copy(c)
        if q is not None:
            q.d = p
    return [VAR_CONS, r]

def f_set_carj(*args):
    fargc_must_eq("set-car!", args, 2)
    fargt_must_in("set-car!", args, 0, (VAR_CONS, VAR_LIST, VAR_NONLIST))
    if args[0][0] == VAR_CONS:
        args[0][1].a = args[1]
    else:
        args[0][1][0] = args[1]
    return [VAR_VOID]

def f_set_cdrj(*args):
    fargc_must_eq("set-cdr!", args, 2)
    fargt_must_in("set-cdr!", args, 0, (VAR_CONS, VAR_LIST, VAR_NONLIST))
    if args[1][0] in (VAR_LIST, VAR_NONLIST):
        args[1][1] = to_cons(args[1])
        args[1][0] = VAR_CONS
    if args[1][0] == VAR_CONS:
        d = args[1][1]
    else:
        d = args[1]
    if args[0][0] == VAR_CONS:
        args[0][1].d = d
        return [VAR_VOID]
    if args[0][0] in (VAR_LIST, VAR_NONLIST):
        a = args[0][1][0]
    else:
        a = args[0]
    args[0][1] = Cons(a, d)
    args[0][0] = VAR_CONS
    return [VAR_VOID]

def f_list_tail(*args):
    fn = "list-tail"
    fargc_must_eq(fn, args, 2);
    fargt_must_eq(fn, args, 1, VAR_NUM)
    fargt_must_in(fn, args, 0, (VAR_CONS, VAR_LIST))
    n = args[1][1]
    r = to_cons(args[0])
    args[0][1] = r
    args[0][0] = VAR_CONS
    for i in range(n):
        if not is_cons(r):
            warning("list-tail overrun")
            break
        r = r.d
    return [VAR_CONS, r]

def f_list_setj(*args):
    fn = "list-set!"
    fargc_must_eq(fn, args, 3);
    fargt_must_eq(fn, args, 1, VAR_NUM)
    fargt_must_in(fn, args, 0, (VAR_CONS, VAR_LIST))
    n = args[1][1]
    if args[0][0] == VAR_LIST:
        args[0][1][n] = args[2]
    else:
        c = f_list_tail(*args[:2])
        assert c[0] == VAR_CONS
        if c[1] is not None:
            c[1].a = args[2]
    return [VAR_VOID]

def f_reverse(*args):
    fargt_must_in("reverse", args, 0, (VAR_CONS, VAR_LIST))
    r = f_list_copy(args)
    r[1].reverse()
    return r

def f_take(*args):
    fargt_must_in("take", args, 1, (VAR_CONS, VAR_LIST))
    fargt_must_eq("take", args, 0, VAR_NUM)
    n = args[0][1]
    if args[1][0] == VAR_LIST:
        return [VAR_LIST, args[1][1][:n]]
    return args[1][1].xcopy(n)

def f_pluss(*args):
    r = 0
    for x in args:
        fchk_or_fail(x[0] == VAR_NUM, "+ expects number")
        r += x[1]
    return [VAR_NUM, r]

def f_minus(*args):
    fargt_must_eq("minus", args, 0, VAR_NUM)
    if len(args) == 1:
        return [VAR_NUM, -args[0][1]]
    r = args[0][1]
    for x in args[1:]:
        fchk_or_fail(x[0] == VAR_NUM, "- expects number")
        r -= x[1]
    return [VAR_NUM, r]

def f_multiply(*args):
    r = 1
    for x in args:
        fchk_or_fail(x[0] == VAR_NUM, "* expects number")
        r *= x[1]
    return [VAR_NUM, r]

def div(args):
    fargt_must_eq("div", args, 0, VAR_NUM)
    n = args[0][1]
    d = 1
    for x in args[1:]:
        fchk_or_fail(x[0] == VAR_NUM, "div expects number")
        d *= x[1]
        if d > n * 2:
            break
    return n, d

def f_divide(*args):
    n, d = div(args)
    return [VAR_NUM, n // d]

def f_div(*args):
    n, d = div(args)
    return [VAR_LIST,
            [[VAR_NUM, n // d],
            [VAR_NUM, n % d]]]

def f_max(*args):
    fargt_must_eq("max", args, 0, VAR_NUM)
    r = args[0][1]
    for x in args[1:]:
        fchk_or_fail(x[0] == VAR_NUM, "max expects number")
        if x[1] > r:
            r = x[1]
    return [VAR_NUM, r]

def f_min(*args):
    fargt_must_eq("min", args, 0, VAR_NUM)
    assert len(args) >= 1
    assert args[0][0] == VAR_NUM
    r = args[0][1]
    for x in args[1:]:
        fchk_or_fail(x[0] == VAR_NUM, "min expects number")
        if x[1] < r:
            r = x[1]
    return [VAR_NUM, r]

def f_abs(*args):
    fargt_must_eq("abs", args, 0, VAR_NUM)
    return [VAR_NUM, abs(args[0][1])]

def n1_pred(args, pred):
    fargt_must_eq("<predicate>", args, 0, VAR_NUM)
    return [VAR_BOOL, pred(args[0][1])]

def f_zerop(*args):
    return n1_pred(args, lambda x: x == 0)

def f_positivep(*args):
    return n1_pred(args, lambda x: x > 0)

def f_negativep(*args):
    return n1_pred(args, lambda x: x < 0)

def f_evenp(*args):
    return n1_pred(args, lambda x: x % 2 == 0)

def f_oddp(*args):
    return n1_pred(args, lambda x: x % 2 == 1)

def n2_pred(args, pred):
    r = True
    if len(args) > 1:
        fchk_or_fail(args[0][0] == VAR_NUM, "<pred> expects number")
        n = args[0][1]
        for x in args[1:]:
            fchk_or_fail(x[0] == VAR_NUM, "<pred>.. expects number")
            if not pred(n, x[1]):
                r = False
                break
            n = x[1]
    return [VAR_BOOL, r]

def f_eq(*args):
    return n2_pred(args, lambda x, y: x == y)

def f_lt(*args):
    return n2_pred(args, lambda x, y: x < y)

def f_gt(*args):
    return n2_pred(args, lambda x, y: x > y)

def f_lte(*args):
    return n2_pred(args, lambda x, y: x <= y)

def f_gte(*args):
    return n2_pred(args, lambda x, y: x >= y)

def f_setj(*args):
    fargc_must_eq("set!", args, 2);
    if args[0][0] != args[1][0]:
        warning("set! different type")
    args[0][0] = args[1][0]
    args[0][1] = args[1][1]
    return [VAR_VOID]

def f_eqp(*args):
    fargc_must_eq("eq?", args, 2);
    if args[0][0] != args[1][0]:
        return [VAR_BOOL, False]
    if args[0][0] in (VAR_LIST, VAR_NONLIST):
        return [VAR_BOOL, (len(args[0][1]) == 0 and len(args[1][1]) == 0)
                or (id(args[0][1]) == id(args[1][1]))]
    if args[0][0] in (VAR_CONS, VAR_DICT, VAR_FUN, VAR_FUN_DOT):
        return [VAR_BOOL, id(args[0][1]) == id(args[1][1])]
    return [VAR_BOOL, args[0][1] == args[1][1]]

def f_equalp(*args):
    fargc_must_eq("equal?", args, 2);
    # note:  DICT do not undergo value-comparison (defined)
    if (args[0][0] not in (VAR_LIST, VAR_NONLIST, VAR_CONS)
            or args[1][0] not in (VAR_LIST, VAR_NONLIST, VAR_CONS)):
        return f_eqp(*args)
    r = [VAR_BOOL, False]
    if ((args[0][0] == VAR_LIST and args[1][0] == VAR_NONLIST)
            or (args[0][0] == VAR_NONLIST and args[1][0] == VAR_LIST)):
        return r
    if args[0][0] == VAR_CONS and args[1][0] == VAR_CONS:
        x = args[0][1]
        y = args[1][1]
        while x is not None:
            if not (y is not None and f_equalp(x.a, y.a)[1]):
                return r
            x = x.d
            y = y.d
        return [VAR_BOOL, y is None]
    a = normal_list(args[0][1])
    b = normal_list(args[1][1])
    if len(a) != len(b):
        return r
    for i in range(len(a)):
        r = f_equalp(a[i], b[i])
        if not r[1]:
            break
    return r

class Dict:
    def __init__(self, w):
        self.t = w[0][0][0]
        self.d = dict([(x[1], y) for x, y in w if x[0] == self.t])

    def ditems(self):
        return [([self.t, k], v) for k, v in self.d.items()]

def f_alist_to_dict(*args):
    fargt_must_in("alist->dict", args, 0, (VAR_LIST, VAR_CONS))
    d = []
    for x in normal_list(args[0][1]):
        if x[0] == VAR_CONS:
            a = x[1].a
            b = x[1].d
        elif x[0] in (VAR_NONLIST, VAR_LIST):
            a = x[1][0]
            b = x[1][1]
        else:
            raise SchemeRunError("not alist")
        d.append((a, b))
    if not d:
        return [VAR_DICT, None]
    return [VAR_DICT, Dict(d)]

def f_dict_to_alist(*args):
    fargt_must_eq("dict->alist", args, 0, VAR_DICT)
    d = [] if args[0][1] is None else args[0][1].ditems()
    return [VAR_LIST, [[VAR_NONLIST, [a, b]]
        for a, b in d]]

def f_dict_setj(*args):
    fn = "dict-set!"
    fargt_must_eq(fn, args, 0, VAR_DICT)
    fargc_must_eq(fn, args, 3)
    if args[0][1] is None:
        args[0][1] = Dict([[args[1], args[2]]])
        return [VAR_VOID]
    if args[0][1].t != args[2][0]:
        raise SchemeRunError("%s: value type %s not dict type %s"
                % (fn, fargt_repr(args[2][0].t), fargt_repr(args[0][1].t)))
    if args[0][1].t != args[1][0]:
        raise SchemeRunError("%s: key type %s not dict type %s"
                % (fn, fargt_repr(args[1][0].t), fargt_repr(args[0][1].t)))
    v = args[0][1].d[args[1][1]] = args[2]
    return [VAR_VOID]

def f_dict_get_defaultj(*args):
    fn = "dict-get-default!"
    fargt_must_eq(fn, args, 0, VAR_DICT)
    fargc_must_eq(fn, args, 3)
    if args[0][1] is None:
        args[0][1] = Dict([[args[1], args[2]]])
        return args[2]
    if args[0][1].t != args[2][0]:
        raise SchemeRunError("%s: default type %s not dict type %s"
                % (fn, fargt_repr(args[2][0].t), fargt_repr(args[0][1].t)))
    if args[0][1].t != args[1][0]:
        raise SchemeRunError("%s: key type %s not dict type %s"
                % (fn, fargt_repr(args[1][0].t), fargt_repr(args[0][1].t)))
    try:
        v = args[0][1].d[args[1][1]]
    except KeyError:
        v = args[0][1].d[args[1][1]] = args[2]
    return v

def f_dict_if_get(*args):
    fn = "dict-if-get"
    fargt_must_eq(fn, args, 0, VAR_DICT)
    fargt_must_in(fn, args, 3, (VAR_FUN, VAR_FUN_DOT))
    if args[0][1] is None:
        return applyx([args[3], v])
    if args[0][1].t != args[1][0]:
        v = args[2]
    try:
        v = args[0][1].d[args[1][1]]
    except KeyError:
        v = args[2]
    return applyx([args[3], v])

def f_dictp(*args):
    return typep(args, VAR_DICT)

class Record:
    def __init__(self, nam, values):
        self.nam = nam
        self.values = values

def f_make_record(*args):
    return [VAR_REC, Record(args[0][1], list(args))]

def f_record_get(*args):
    fn = "record-get"
    fargt_must_eq(fn, args, 0, VAR_REC)
    fargt_must_eq(fn, args, 1, VAR_NUM)
    return args[0][1].values[args[1][1]]

def f_record_setj(*args):
    fn = "record-set!"
    fargt_must_eq(fn, args, 0, VAR_REC)
    fargt_must_eq(fn, args, 1, VAR_NUM)
    args[0][1].values[args[1][1]] = args[2]
    return [VAR_VOID]

def f_recordp(*args):
    fn = "record?"
    fargt_must_eq(fn, args, 0, VAR_REC)
    fargt_must_eq(fn, args, 1, VAR_NAM)
    return [VAR_BOOL, args[0][1].nam == args[1][1]]

def f_string_ref(*args):
    fn = "string-ref"
    fargc_must_eq(fn, args, 2);
    fargt_must_eq(fn, args, 1, VAR_NUM)
    fargt_must_eq(fn, args, 0, VAR_STRING)
    return [VAR_NUM, args[0][1][args[1][1]]]

def f_string_to_list(*args):
    fargt_must_eq("string->list", args, 0, VAR_STRING)
    return [VAR_LIST, [[VAR_NUM, ord(c)] for c in args[0][1]]]

def f_list_to_string(*args):
    fargt_must_eq("list->string", args, 0, VAR_LIST)
    return [VAR_STRING, "".join([chr(n[1]) for n in args[0][1]])]

def f_symbol_to_string(names):
    def to_name_string(*args):
        fargt_must_eq("symbol->string", args, 0, VAR_NAM)
        return [VAR_STRING, names[args[0][1]]]
    return to_name_string

def f_substring(*args):
    fargt_must_eq("substring", args, 0, VAR_STRING)
    fargt_must_eq("substring", args, 1, VAR_NUM)
    s = args[0][1]
    i = args[1][1]
    if len(args) >=3:
        fargt_must_eq("substring", args, 2, VAR_NUM)
        j = args[2][1]
        r = s[i:j]
    else:
        r = s[i:]
    return [VAR_STRING, r]

def f_string_append(*args):
    r = []
    for (i, a) in enumerate(args):
        if a[0] != VAR_STRING:
            raise SchemeRunError("string-append args[%d] got %s"
                % (i, fargt_repr(a[0])))
        r.append(a[1])
    return [VAR_STRING, "".join(r)]

def f_stringeqp(*args):
    fargt_must_eq("string=?", args, 0, VAR_STRING)
    fargt_must_eq("string=?", args, 1, VAR_STRING)
    return [VAR_BOOL, args[0][1] == args[1][1]]

def f_stringltp(*args):
    fargt_must_eq("string<?", args, 0, VAR_STRING)
    fargt_must_eq("string<?", args, 1, VAR_STRING)
    return [VAR_BOOL, args[0][1] < args[1][1]]

def f_stringgtp(*args):
    fargt_must_eq("string>?", args, 0, VAR_STRING)
    fargt_must_eq("string>?", args, 1, VAR_STRING)
    return [VAR_BOOL, args[0][1] > args[1][1]]

def typep(args, t):
    fargc_must_eq("<t-pred>", args, 1);
    return [VAR_BOOL, t == args[0][0]]

def f_booleanp(*args):
    return typep(args, VAR_BOOL)

def f_numberp(*args):
    return typep(args, VAR_NUM)

def f_procedurep(*args):
    r = typep(args, VAR_FUN)
    if not r[1]:
        r = typep(args, VAR_FUN_DOT)
    return r

def f_symbolp(*args):
    return typep(args, LEX_NAM)

def f_nullp(*args):
    r = typep(args, VAR_LIST)
    if r[1]:
        r[1] = (0 == len(args[0][1]))
    elif args[0][0] == VAR_CONS:
        r[1] = (None == args[0][1])
    return r

def f_listp(*args):
    def get_last(c):
        while c.d is not None:
            if not is_cons(c.d):
                return c
            c = c.d
        return c
    r = typep(args, VAR_LIST)
    if r[1]:
        r[1] = (0 != len(args[0][1]))
    elif args[0][0] == VAR_CONS:
        r[1] = not (args[0][1] is None
                or get_last(args[0][1]).d is not None)
    return r

def f_pairp(*args):
    r = typep(args, VAR_NONLIST)
    if r[1]:
        return r
    elif args[0][0] == VAR_CONS:
        r[1] = args[0][1] is not None
        return r
    return f_listp(args)

def f_not(*args):
    fargc_must_eq("not", args, 1);
    return [VAR_BOOL, args[0] == [VAR_BOOL, False]]

def f_display(names):
    def display(*args):
        for a in args:
            if a[0] == VAR_STRING:
                s = a[1]
            else:
                s = vrepr(a, names)
            sys.stdout.write(s)
        return [VAR_VOID]
    return display

def f_newline(*args):
    fargc_must_eq("newline", args, 0);
    sys.stdout.write("\n")
    return [VAR_VOID]

def f_length(*args):
    fargt_must_in("length", args, 0, (VAR_CONS, VAR_LIST))
    if args[0][0] == VAR_CONS:
        if args[0][1] is None:
            r = 0
        else:
            r = args[0][1].length()
    else:
        r = len(args[0][1])
    return [VAR_NUM, r]

def f_apply(*args):
    fargt_must_in("apply", args, 1, (VAR_CONS, VAR_LIST))
    return applyx([args[0]] + normal_list(args[1][1]))

def f_map(*args):
    fargt_must_in("map", args, 0, (VAR_FUN, VAR_FUN_DOT))
    f = args[0]
    inputs = []
    for j in range(1, len(args)):
        fchk_or_fail(args[j][0] in (VAR_LIST, VAR_CONS),
                "map expects list")
        inputs.append(normal_list(args[j][1]))
    n = min(len(y) for y in inputs)
    r = []
    for i in range(n):
        w = []
        for j in range(len(inputs)):
            w.append(inputs[j][i])
        r.append(applyx([f] + w))
    return [VAR_LIST, r]

def search_pred(a):
    if a[0] in (VAR_FUN, VAR_FUN_DOT):
        p = lambda x: fun_call([a, x])
    else:
        p = lambda x: f_equalp(a, x)
    def t(y):
        v = p(y)
        return v[0] != VAR_BOOL or v[1]
    return t

def f_member(*args):
    f = [VAR_BOOL, False]
    fargt_must_in("member", args, 1, (VAR_CONS, VAR_LIST, VAR_NONLIST))
    t = search_pred(args[0])
    if args[1][0] == VAR_CONS:
        if args[1][1] is None:
            return f
        if t(args[1][1].a):
            return args[1]
    else:
        if len(args[1][1]) == 0:
            return f
        if t(args[1][1][0]):
            # note: in this case list need not be converted to cons
            #       -- we merely get another ref to this list as-is
            return args[1]
    r = f_cdr(args[1])[1]
    while r:
        if not is_cons(r):
            warning("member hit non-cons cdr")
            if t(r):
                return r
            break
        if t(r.a):
            return [VAR_CONS, r]
        r = r.d
    return f

# as opposed to member, assoc does not need to convert its
# argument to a VAR_CONS as no reference into list is handed out
def f_assoc(*args):
    f = [VAR_BOOL, False]
    fargt_must_in("assoc", args, 1, (VAR_CONS, VAR_LIST, VAR_NONLIST))
    t = search_pred(args[0])
    if args[1][0] == VAR_CONS:
        r = args[1][1]
        while r:
            if not is_cons(r):
                warning("assoc hit non-cons cdr")
                break
            c = r.a
            if not is_cons(c):
                warning("assoc traversed non-cons car")
                continue
            if t(c.a):
                return [VAR_CONS, c]
            r = r.d
    else:
        for x in args[1][1]:
            if t(f_car(x)):
                return x
    return f

def f_error(names):
    def err(*args):
        f_display([VAR_LIST, args])
        sys.stderr.write("error-function invoked --\n")
        for i, a in enumerate(args):
            sys.stdout.write("error-args[%d]: %s\n" % (i, vrepr(a, names)))
        sys.stderr.write("-- sorry'bout that\n")
        sys.exit(2)
    return err

#
# the core
#

def is_last(w, block):
    return id(w) == id(block[-1])

def fun_call(a):
    debug("stack-apply", a)
    # note: ^ debug-log looks as about to perform call,
    # but in fact as already in fun_call here
    # -- already chosen to invoke *a* on-stack
    x = a[0]
    args = a[1:]
    # note: native function call, for builtins
    if len(x) == 2:
        return x[1](*args)
    # otherwise, tail-call-optimize
    dot = x[0] == VAR_FUN_DOT
    captured = x[1]
    parms = x[2]
    block = x[3]
    done = False
    while not done:
        env = dict(captured)
        if dot:
            last = len(parms) - 1
            if len(args) < last:
                raise SchemeRunError("fun-dot expected %d args got %d"
                        % (len(parms), len(args)))
            for i in range(last):
                env[parms[i]] = args[i]
            env[parms[last]] = [VAR_LIST, list(args[last:])]
        else:
            if len(args) != len(parms):
                raise SchemeRunError("fun expected %d args got %d"
                        % (len(parms), len(args)))
            for i, v in enumerate(args):
                env[parms[i]] = v
        done = True
        for w in block:
            v = xeval(w, env)
            if v[0] == VAR_APPLY:
                a = v[1]
                assert a[0][0] in (VAR_FUN, VAR_FUN_DOT)
                if not is_last(w, block):
                    v = fun_call(a)
                else:
                    debug("iter-apply", a)
                    args = a[1:]
                    captured, parms, block = a[0][1:]
                    dot = a[0][0] == VAR_FUN_DOT
                    done = False
    return v

def make_fun(up, x, fun_dot):
    fun_parms = x[0]
    fun_captured = dict()
    for k in x[1]:
        fun_captured[k] = up[k]
    fun_block = x[2:]
    t = VAR_FUN_DOT if fun_dot else VAR_FUN
    return [t, fun_captured, fun_parms, fun_block]

def rebind(fun_env, ids, env):
    for i, v in fun_env.items():
        if v[0] == LEX_REF:
            assert v[1] == i
            assert i in ids
            debug("bind", i, env[i])
            fun_env[i] = env[i]
        # note: not needed to recurse on VAR_LIST
        # from here -- rebind_r(v[1], ids, env)

def rebind_r(s, ids, env):
    for x in s:
        if x[0] in (VAR_FUN, VAR_FUN_DOT):
            rebind(x[1], ids, env)
        if x[0] == VAR_LIST:
            rebind_r(x[1], ids, env)

def rebind_args(ids, env):
    for i in ids:
        v = env[i]
        if v[0] in (VAR_FUN, VAR_FUN_DOT):
            rebind(v[1], ids, env)
        if v[0] == VAR_LIST:
            rebind_r(v[1], ids, env)

def flatten_splices(a):
    r = []
    for v in a:
        if v[0] == VAR_SPLICE:
            r.extend(v[1])
        else:
            r.append(v)
    return r

def run_each(x, env):
    return [run(y, env) for y in x]

i_env = {}
# initial subset of env -- conceptually part of

# note: an applicable result is deferred and a
# VAR_APPLY is returned instead -- to be applied --
# when it is running the a syntax block (not for
# native builtins) -- this is for tail-call-optimization.
def xeval(x, env):
    debug("eval", x)
    if type(x) != list:
        if x[0] == LEX_LIST:
            r = flatten_splices(run_each(x[1], env))
            return [VAR_LIST, r]
        if x[0] == LEX_NONLIST:
            r = flatten_splices(run_each(x[1], env))
            if r[-1][0] == VAR_LIST:
                a = r[-1][1]
                return [VAR_LIST, r[:-1] + a]
            if r[-1][0] == VAR_CONS:
                c = Cons.from_list(r[:-1])
                c.d = r[-1][1]
                return [VAR_CONS, c]
            return [VAR_NONLIST, r]
        if x[0] == LEX_SPLICE:
            r = run(x[1], env)
            return [VAR_SPLICE, normal_list(r[1])]
        if x[0] == LEX_NAM:
            return env[x[1]]
        if x[0] == LEX_NUM:
            return [VAR_NUM, x[1]]
        if x[0] == LEX_BOOL:
            return [VAR_BOOL, x[1]]
        if x[0] == LEX_STRING:
            return [VAR_STRING, x[1]]
        if x[0] == LEX_SYM:
            return [VAR_NAM, x[1]]
        if x[0] == LEX_QUOTE:
            return [VAR_QUOTE, x[1]]
        if x[0] == LEX_UNQUOTE:
            return [VAR_UNQUOTE, x[1]]
        if x[0] == LEX_QUASIQUOTE:
            return [VAR_QUOTE, x[1]]
        if x[0] == LEX_REF:
            return x  # sticks till OP_REBIND
        if x[0] == LEX_VOID:
            return [VAR_VOID]
        if x[0] == LEX_DOT:
            raise SchemeRunError("invalid use of dot")
        # in some cases we could have catched at parse time
        # which means it would have been a lex error.
        broken(x)
    assert type(x) == list and len(x)
    if x[0] == OP_DEFINE:
        env[x[1]] = run(x[2], env)
        return [VAR_VOID]
    if x[0] in (OP_LAMBDA, OP_LAMBDA_DOT):
        return make_fun(env, x[1:], x[0] == OP_LAMBDA_DOT)
    if x[0] == OP_COND:
        for y in x[1:]:
            t = run(y[0], env)
            if t[0] != VAR_BOOL or t[1]:
                return xeval(y[1], env)
        raise SchemeRunError("all cond #f")
    if x[0] == OP_REBIND:
        rebind_args(x[1], env)
        return [VAR_VOID]
    if x[0] == OP_IMPORT:
        e = ExtraDict(i_env)
        for z in x[2:]:
            run(z, e)
        for (a, b) in x[1].items():
            if b not in e:
                # note: could instead have been checked on parse
                #       of the import by performing unbound there
                raise SchemeRunError("no %s for export" % (names[b],))
            env[a] = e[b]
        return [VAR_VOID]
    if x[0] == OP_EXPORT:
        # idea: feature when running a library
        return [VAR_VOID]
    if x[0] == OP_SEQ:
        for y in x[1:]:
            r = run(y, env)
        return r
    a = run_each(x, env)
    return xapply(a)

def xapply(a):
    if a[0][0] not in (VAR_FUN, VAR_FUN_DOT):
        raise SchemeRunError("apply %s"
                % (fargt_repr(a[0][0])))
    if is_fun_lex(a[0]):
        return [VAR_APPLY, a]
    return fun_call(a)

def run(x, env):
    a = xeval(x, env)
    return fcall(a)

def applyx(a):
    return fcall(xapply(a))

def fcall(a):
    while a[0] == VAR_APPLY:
        if a[1][0][0] not in (VAR_FUN, VAR_FUN_DOT):
            broken("not fun")
        a = fun_call(a[1])
    return a

#
# init
#

def with_new_name(name, f, d, names):
    i = len(names)
    if name in names:
        beoken("not new")
    names.append(name)
    d[i] = f

def init_macros(env, names):
    macros = dict()
    macros[NAM_QUOTE] = m_quote
    macros[NAM_QUASIQUOTE] = m_quasiquote
    macros[NAM_UNQUOTE] = m_unquote
    macros[NAM_UNQUOTE_SPLICE] = m_unquote_splice
    macros[NAM_MACRO] = m_macro(macros)
    with_new_name("gensym", m_gensym(names), macros, names)
    with_new_name("define", m_define, macros, names)
    with_new_name("seq", m_seq, macros, names)
    with_new_name("lambda", m_lambda, macros, names)
    with_new_name("case-lambda", m_case_lambda, macros, names)
    with_new_name("let", m_let, macros, names)
    with_new_name("let*", m_letx, macros, names)
    with_new_name("letrec", m_letrec, macros, names)
    with_new_name("letrec*", m_letrecx, macros, names)
    with_new_name("cond", m_cond, macros, names)
    with_new_name("case", m_case, macros, names)
    with_new_name("do", m_do, macros, names)
    with_new_name("begin", m_begin, macros, names)
    with_new_name("if", m_if, macros, names)
    with_new_name("and", m_and, macros, names)
    with_new_name("or", m_or, macros, names)
    with_new_name("when", m_when, macros, names)
    with_new_name("unless", m_unless, macros, names)
    with_new_name("export", m_export, macros, names)
    with_new_name("scope", m_scope, macros, names)
    with_new_name("import", m_import(names, macros), macros, names)
    return macros

def init_env(names):
    env = dict()
    env[NAM_CAR] = [VAR_FUN, f_car]
    env[NAM_EQVP] = [VAR_FUN, f_eqp]  # impl: eq? is identical as eqv?
    env[NAM_LENGTH] = [VAR_FUN, f_length]
    env[NAM_APPLY] = [VAR_FUN, f_apply]
    env[NAM_LIST] = [VAR_FUN, f_list]
    env[NAM_NONLIST] = [VAR_FUN, f_nonlist]
    with_new_name("+", [VAR_FUN, f_pluss], env, names)
    with_new_name("-", [VAR_FUN, f_minus], env, names)
    with_new_name("*", [VAR_FUN, f_multiply], env, names)
    with_new_name("/", [VAR_FUN, f_divide], env, names)
    with_new_name("div", [VAR_FUN, f_div], env, names)
    with_new_name("zero?", [VAR_FUN, f_zerop], env, names)
    with_new_name("negative?", [VAR_FUN, f_negativep], env, names)
    with_new_name("positive?", [VAR_FUN, f_positivep], env, names)
    with_new_name("even?", [VAR_FUN, f_evenp], env, names)
    with_new_name("odd?", [VAR_FUN, f_oddp], env, names)
    with_new_name("list-copy", [VAR_FUN, f_list_copy], env, names)
    with_new_name("list-ref", [VAR_FUN, f_list_ref], env, names)
    with_new_name("list-tail", [VAR_FUN, f_list_tail], env, names)
    with_new_name("list-set!", [VAR_FUN, f_list_setj], env, names)
    with_new_name("reverse", [VAR_FUN, f_reverse], env, names)
    with_new_name("take", [VAR_FUN, f_take], env, names)
    with_new_name("member", [VAR_FUN, f_member], env, names)
    with_new_name("assoc", [VAR_FUN, f_assoc], env, names)
    with_new_name("set!", [VAR_FUN, f_setj], env, names)
    with_new_name("eq?", [VAR_FUN, f_eqp], env, names)
    with_new_name("equal?", [VAR_FUN, f_equalp], env, names)
    with_new_name("cdr", [VAR_FUN, f_cdr], env, names)
    with_new_name("set-car!", [VAR_FUN, f_set_carj], env, names)
    with_new_name("set-cdr!", [VAR_FUN, f_set_cdrj], env, names)
    with_new_name("append", [VAR_FUN, f_append], env, names)
    with_new_name("cons", [VAR_FUN, f_cons], env, names)
    with_new_name("boolean?", [VAR_FUN, f_booleanp], env, names)
    with_new_name("number?", [VAR_FUN, f_numberp], env, names)
    with_new_name("procedure?", [VAR_FUN, f_procedurep], env, names)
    with_new_name("symbol?", [VAR_FUN, f_symbolp], env, names)
    with_new_name("null?", [VAR_FUN, f_nullp], env, names)
    with_new_name("list?", [VAR_FUN, f_listp], env, names)
    with_new_name("pair?", [VAR_FUN, f_pairp], env, names)
    with_new_name("not", [VAR_FUN, f_not], env, names)
    with_new_name("=", [VAR_FUN, f_eq], env, names)
    with_new_name("<", [VAR_FUN, f_lt], env, names)
    with_new_name(">", [VAR_FUN, f_gt], env, names)
    with_new_name("<=", [VAR_FUN, f_lte], env, names)
    with_new_name(">=", [VAR_FUN, f_gte], env, names)
    with_new_name("display", [VAR_FUN, f_display(names)], env, names)
    with_new_name("newline", [VAR_FUN, f_newline], env, names)
    with_new_name("error", [VAR_FUN, f_error(names)], env, names)
    with_new_name("map", [VAR_FUN, f_map], env, names)
    with_new_name("abs", [VAR_FUN, f_abs], env, names)
    with_new_name("alist->dict", [VAR_FUN, f_alist_to_dict], env, names)
    with_new_name("dict->alist", [VAR_FUN, f_dict_to_alist], env, names)
    with_new_name("dict-set!", [VAR_FUN, f_dict_setj], env, names)
    with_new_name("dict-get-default!", [VAR_FUN, f_dict_get_defaultj], env, names)
    with_new_name("dict-if-get", [VAR_FUN, f_dict_if_get], env, names)
    with_new_name("dict?", [VAR_FUN, f_dictp], env, names)
    with_new_name("make-record", [VAR_FUN, f_make_record], env, names)
    with_new_name("record-get", [VAR_FUN, f_record_get], env, names)
    with_new_name("record-set!", [VAR_FUN, f_record_setj], env, names)
    with_new_name("record?", [VAR_FUN, f_recordp], env, names)
    with_new_name("string->list", [VAR_FUN, f_string_to_list], env, names)
    with_new_name("list->string", [VAR_FUN, f_list_to_string], env, names)
    with_new_name("string-ref", [VAR_FUN, f_string_ref], env, names)
    with_new_name("substring", [VAR_FUN, f_substring], env, names)
    with_new_name("string-append", [VAR_FUN, f_string_append], env, names)
    with_new_name("string=?", [VAR_FUN, f_stringeqp], env, names)
    with_new_name("string<?", [VAR_FUN, f_stringltp], env, names)
    with_new_name("string>?", [VAR_FUN, f_stringgtp], env, names)
    with_new_name("symbol->string", [VAR_FUN, f_symbol_to_string(names)], env, names)
    return env

def run_top(ast, env, names):
    for a in ast:
        r = run(a, env)
        if r[0] != VAR_VOID:
            result(vrepr(r, names))

def inc_functions(names, env, macros):
    a = []
    for w in itertools.product( * ["ad"] * 2):
        a.append("(define (c%s%sr x) (c%sr (c%sr x)))" % (2 * w))
    for w in itertools.product( * ["ad"] * 3):
        a.append("(define (c%s%s%sr x) (c%sr (c%sr (c%sr x))))" % (2 * w))
    t = parse("\n".join(a), names, macros, env.keys())
    run_top(t, env, names)

def inc_macros(names, env, macros):
    s = """
(macro define@ args
  (define n (length args))
  (define n_1 (- n 1))
  (define syms (take n_1 args))
  (define s (list-ref args n_1))
  (define ls (gensym))
  (define defs (let loop ((i 0) (r '()))
     (if (equal? i n_1) r
       (loop (+ 1 i)
         (cons `(define ,(list-ref syms i) (list-ref ,ls ,i)) r))
     )))
  `(seq (define ,ls ,s) ,@defs)
)
(macro define-record-type (name constructor pred . accessors)
  (define (getter x i)
    `(define (,(cadr x) v) (record-get v ,i)))
  (define getter-defs
    (let loop ((d accessors) (r '()) (i 0))
      (if (null? d) r
        (loop (cdr d) (cons (getter (car d) i) r) (+ i 1))
    )))
  (define (setter x i)
    (if (eq? (length x) 2) #f
      `(define (,(caddr x) v y) (record-set! v ,i y))))
  (define setter-defs
    (let loop ((d accessors) (r '()) (i 0))
      (if (null? d) r
        (let ((e (setter (car d) i)))
          (loop (cdr d) (if e (cons e r) r) (+ i 1))
    ))))
  (define c-name (car constructor))
  `(seq
    (define ,constructor
      (make-record ,@(cdr constructor)))
    (define (,pred v) (record? v ',name))
    ,@getter-defs
    ,@setter-defs
))
"""
    t = parse(s, names, macros, env.keys())
    run_top(t, env, names)

def init_top():
    global filename
    names = [None] * 13
    names[NAM_THEN] = "=>"
    names[NAM_ELSE] = "else"
    names[NAM_QUOTE] = "quote"
    names[NAM_QUASIQUOTE] = "quasiquote"
    names[NAM_UNQUOTE] = "unquote"
    names[NAM_UNQUOTE_SPLICE] = "unquote-splice"
    names[NAM_MACRO] = "macro"
    names[NAM_CAR] = "car"
    names[NAM_EQVP] = "eqv?"
    names[NAM_LENGTH] = "length"
    names[NAM_APPLY] = "apply"
    names[NAM_LIST] = "list"
    names[NAM_NONLIST] = "nonlist"
    env = init_env(names)
    macros = init_macros(env, names)

    filename = "inc-functions"
    inc_functions(names, env, macros)

    global i_env
    i_env = dict(env)

    filename = "inc-macros"
    inc_macros(names, env, macros)

    filename = None
    return names, env, macros

#
# ui
#

def xrepr(s, names):
    if not s:
        return "#<>"
    if type(s) == list:
        if s[0] == OP_DEFINE:
            return "#<define %s %s #>" % (
                    names[s[1]], xrepr(s[2], names))
        if s[0] in (OP_LAMBDA, OP_LAMBDA_DOT):
            return "#<(%s)[%s] %s #>" % (
                    " ".join(names[i] for i in s[1]),
                    " ".join(names[i] for i in s[2]), xrepr(s[3], names))
        if s[0] == OP_REBIND:
            return "#<rebind %s #>" % (" ".join(names[i] for i in s[1]))
        if s[0] == OP_COND:
            return "#<cond%s #>" % (
                    "".join(" [%s %s]" % (xrepr(a, names), xrepr(b, names))
                    for a, b in s[1:]),)
        if s[0] == OP_IMPORT:
            return "#<import {%s} %s #>" % (
                    " ".join(names[i] for i in s[1]),
                    " ".join(xrepr(x, names) for x in s[2:]))
        if s[0] == OP_EXPORT:
            return "#<export%s #>" % (
                    "".join(" " + xrepr(x, names) for x in s[2:]))
        if s[0] == OP_SEQ:
            return "#<seq%s #>" % (
                    "".join(" " + xrepr(x, names) for x in s[1:]))
        return "(%s)" % (" ".join(xrepr(x, names) for x in s),)
    if s[0] in (LEX_LIST, LEX_NONLIST):
        w  = [xrepr(x, names) for x in s[1]]
        if s[0] == LEX_NONLIST:
            w.insert(-1, ".")
        return "$(%s)" % " ".join(w)
    if s[0] == LEX_SPLICE:
        return "#<splice %s #>" % (xrepr(s[1], names),)
    if s[0] == LEX_SYM:
        return "'%s" % (names[s[1]],)
    if s[0] == LEX_NAM:
        return names[s[1]]
    if s[0] == LEX_REF:
        return "##%s" % (names[s[1]],)
    if s[0] == LEX_NUM:
        return str(s[1])
    if s[0] == LEX_STRING:
        return "\"" + s[1] + "\""
    if s[0] == LEX_BOOL:
        if type(s[1]) != bool:
            warning("dirty bool")
        return "#" + "ft"[bool(s[1])]
    if s[0] == LEX_VOID:
        return "#<void>"
    if s[0] == LEX_DOT:
        return "#<dot>"
    if s[0] == LEX_QUOTE:
        return "#<quote %s #>" % (xrepr(s[1], names),)
    if s[0] == LEX_UNQUOTE:
        return "#<unquote %s #>" % (xrepr(s[1], names),)
    return "#< %r #>" % (s,)

def is_fun_lex(s):
    if len(s) == 4:
        return True
    assert len(s) == 2
    return False

def vrepr(s, names):
    if s[0] in (VAR_LIST, VAR_NONLIST):
        w  = [vrepr(x, names) for x in s[1]]
        if s[0] == VAR_NONLIST:
            w.insert(-1, ".")
        return "(%s)" % " ".join(w)
    if s[0] == VAR_SPLICE:
        return "#~splice(%s)" % " ".join(
                vrepr(x, names) for x in s[1])
    if s[0] == VAR_CONS:
        if s[1] is None:
            return "#~()"
        else:
            return "#~%s" % vrepr(s[1].to_list_var(), names)
    if s[0] == VAR_NAM:
        return names[s[1]]
    if s[0] == VAR_NUM:
        return str(s[1])
    if s[0] == VAR_STRING:
        return "\"" + s[1] + "\""
    if s[0] == VAR_DICT:
        if s[1] is None:
            return "#{}"
        return "#{ " + " ".join(
                ["(%s . %s)" % (vrepr(x, names), vrepr(y, names))
                    for x, y in s[1].ditems()]) + " }"
    if s[0] in (VAR_FUN, VAR_FUN_DOT):
        r = "#~fun" if s[0] == VAR_FUN else "#~dotfun"
        if is_fun_lex(s):
            return "%s(%s)[%s]{ %s }" % (r,
                    " ".join(names[i] for i in s[2]),
                    " ".join("%s:%s" % (names[k], vrepr(v, names))
                        for k, v in s[1].items()),
                    " ".join(xrepr(x, names) for x in s[3]))
        else:
            return r
    if s[0] == VAR_FUN_DOT:
        return "#~dotfun"
    if s[0] == VAR_BOOL:
        if type(s[1]) != bool:
            warning("dirty bool")
        return "#" + "ft"[bool(s[1])]
    if s[0] == VAR_VOID:
        return "#~void"
    if s[0] == VAR_QUOTE:
        return "#~quote %s" % xrepr(s[1], names)
    if s[0] == VAR_UNQUOTE:
        return "#~unquote %s" % xrepr(s[1], names)
    return "#~ %r" % (s,)

if __name__ == "__main__":
    if len(sys.argv) == 2:
        names, env, macros = init_top()
        filename = sys.argv[1]
        with open(filename) as f:
            try:
                tree = parse(f.read(), names, macros, env.keys())
            except SchemeSrcError as e:
                sys.stderr.write("error in file: %s\n" % (filename,))
                sys.stderr.write(e.args[0] + "\n")
                sys.exit(1)
            else:
                try:
                    run_top(tree, env, names)
                except SchemeRunError as e:
                    sys.stderr.write("runtime error: %s\n" % (e.args[0],))
                    sys.exit(2)
        sys.exit(0)

    if not sys.stdin.isatty():
        error("missing input file")
        sys.exit(1)
    sys.stdout.write("WELCOME TO HUMBLE SCHEME.\n"
            "End line with ';' to run and with ';;' to show expanded code.\n"
            "Enter a lone ;; on a line to toggle verbose debug.\n"
            "Provide eof indication to exit.\n--\n")
    names, env, macros = init_top()
    buf = []
    while True:
        line = sys.stdin.readline()
        if not line:
            break
        buf.append(line)
        line = line.rstrip()
        if line.endswith(";"):
            if line == ";;":
                if not verbose:
                    verbose = True
                    debug(list(enumerate(names)))
                else:
                    verbose = False
            try:
                t = parse("".join(buf), names, macros, env.keys())
            except SchemeSrcError as e:
                if filename:
                    sys.stderr.write("error in file: %s\n"
                            % (filename,))
                    filename = None
                sys.stderr.write("lexical error: " + e.args[0] + "\n")
            else:
                if line.endswith(";;"):
                    for xt in t:
                        print("%", xrepr(xt, names))
                try:
                    run_top(t, env, names)
                except SchemeRunError as e:
                    sys.stderr.write("error: " + e.args[0] + "\n")
                sys.stdout.flush()  # for evt.display
            buf = []
    sys.stdout.write("fare well.\n")

