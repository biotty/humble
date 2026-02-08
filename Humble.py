#!/usr/bin/env python3
#
# Welcome to "Humble Schemer", copyright Christian Oeien
#
# My scheme is more humble than yours!
#
#
# Deviations:  (concept)
#
# The environment cannot be mutated from an inner scope,
# as (set!) traditionally does.  Only (re-)defining
# a name alters the environment, which can only be done
# at its own lexical scope.  The term "name" here is to
# mean a scoped variable-name, and not simply the
# interned entity (symbol).
# Variable names in the program works as in usual scheme.
# Now a crucial difference with traditional scheme:
# Parameter values to a procedure are not copied,
# but taken "by reference".  I have (alias? a b) to report
# whether a and b refers to the same object/lvalue.
# The let(rec)(*) macros, being constructed of lambda,
# reflects the by-reference passing (in the rare case needed
# we may duplicate explicitly as mentioned below).
#
# To facilitate value-semantics clone (such as use of
# a number) and facilitate for separation of lvalues I provide
# (dup) and (local).  I have (ref) that only binds to the given
# object, as well as (define) with traditional semantics;
# performs (dup) when more references exist.
# Surprises with references will only occur
# due to mutation.  Given this layout of names and values,
# I allow mutation of the variables themselves as "lvalues"
# and I drop the mutation of set! that replaces on the
# location of a name (list-set! is not dropped).  The
# rationale is that scheme already operates has such mutation,
# when you hold cons-cells or lambda-captures.
# There is both simplification and practical benefit in
# generalizing mutation to "values" but eliminating remote
# mutation on the environments.
# A local name can only be re-defined in
# the very same scope -- as opposed to traditional scheme.
# (def) is a convenience that does ref and dup.
# the dup implementation is required to never take a copy
# when not needed (only one reference held to the object).
#
# Now that the warning about variable-names, reference
# semantics and the local environment has been given, I
# re-introduce set! because a name like setv! is noisy.
#
# Features:  (language)
#
# * Checked parens-pairs (), [] and {} - no meanings
# * Optimized "tail call" - impl/w deferred-apply var
# * Contigous (non)lists until sharing, on which list-
#     variable transforms to a cons chain.  sharing
#     happens on cdr-usage or otherhow that more than
#     one variable owns any of head or tail.
# * The dict type (an alist with efficient repr)
# * macro (exactly as lisp defmacro) -- and gensym
# * import of file (that needs "export" as first expr)
# * ref is as define (allows (recursive) function syntax)
# * define allows usage of same (previous) name in expression
# * ref@ -- define with list values (no multi-value)
# * seq -- lexical block without own scope
# * scope -- an import with contents, not loading file
#     -- export certain names from "here-doc"
# * Record type, backing traditional define-record-type.
# * A nonlist function (like list).
# * ,@ propperly separated as two independent operators.
# * Functions and special forms as specified in r7rs
#     but limited for math and a very basic IO:
#     input/output-file, r/w-byte, read-line, write-string
#     eof-object? input-pipe output-pipe exit
# * IO is utf-8 and there is no "file-mode".
# * For byte access there are byte-io functions.
# * char- and string-literals with a few escapes
#     and otherwise rely on source encoding -- utf-8
# * in-string and out-string provide port interface
# * #void -- unit type i-e is returned on r7rs "unspec"
# * (error) function that reports given value and exits.
#
# Work-in-Progress:  (elaboration)
#
# * "handlers" global env entry (like stdout) that also each
#   host-fun has a ref to, will be tried for symbol-key on
#   (error).  function will get invoked with error args,
#   which is for host-functions the args, and the handler
#   may mutate these args.  the bultin functions will
#   call the handler of its name when arg types are
#   invalid or other situation, and re-try unless yields #f,
#   or if re-try fails on same reason, terminating as now.
# * Label form set symbol on scope, which may then be used
#   in a break statement.  This is implemented by flagging
#   interpreter state by the break symbol, which inhibits
#   evaluation until form of respective label(s) observed,
#   dynamically, with syntax i-e (case-except (raise '(foo 1))
#   (('foo 'bar) 1)) where car is implicit and the default else
#   is a no-op.  or general handle-except that takes a lambda
#   and yields #f iff to consider still not catched.  the flag
#   state may be checked only in run_each().
# * The c++ implementation will use engine registering own
#   sets of macros and functions.  One such set may be
#   ncurses as of the snake demo, but another more interesting
#   set of functionality may be libffi support - that also
#   invites looking into support of an engine-opaque VAR
#   type where only a user-data and destructor-callback is known.
# * Functions available untangling the repl components,
#   (read port), (eval d) also does macro_expand and run
#   in global env, (write d port) and (compile d port).
#   These leverage from_lex and to_lex, as d is a var,
#   while read then benefits from lex parse.
#   This results in two types of files, x: lex not macro-expanded,
#   and may save either data or code, and y: op-code-lex ready
#   to be fed to the interpreter and run.  There is no conversion
#   from type y, on which only action is to run.  Remember that
#   I still don't have string->symbol, which would not trivially
#   work in a scope, and this problem is not worth resolving.
# * Delirium ad previous idea:  One could have a graft-point in the
#   AST denoted by an expression, that is globally named and may
#   then be used as a target p for (compile p), allowing to mutate
#   the code itself.  The graft point may be wrapped as possibly
#   a lambda or OP_IMPORT (at hand waving level of thought here).
#
# Excluded:  (non-features)
#
# * Multi-value, eval, call/cc, dyn-param, except, force-delay
# * str->sym (because parse-time intern of all names)
# * Implicit quoting in case (I evaluate (in short circuiting
#   fashion) the <datum>s)
# * Other commenting than ; and #| .. |#
# * Label-syntax.  So a list with a loop cannot be expressed.
#   (display) of data with loops will not work.  And it only
#   un-repr string on top-level, seperating args with LWS.
# * Float, exact, complex, char (but only integral numbers)
# * No |n a m e s| and no fold-case mode.  Recommended restriction
#   to ascii names.  But utf8 accepted in char- or string-literal.
#   Names cannot start in a '.' or a '@' but may contain them.
# * FS/System ops, as there is i.e input-pipe
# * define-syntax; instead powerful "unhygienic" lisp macro
# * Char, but number parsed for #\ and with utf-8 support.
# * Only octal escape for bytes in string.
# * (newline).
#
# Note:  (beware)
#
# * The datums in a case are evaluated, when they are as of
#     r7rs implicitly to be quoted.
# * cont?? and #void shall be considered non-existent and
#     provided merely for inspection in development of a program.
#     They are peek-under-the-hood facilities.  Likewise,
#     make-record is not expected to be useful by itself.
#
# Implementation-defined:  (detail)
#
# '() may naturally create an instance of an empty list.  This
# is a internal representation detail, and not observed except
# for (as always for list/cons transitions) with cont??
# Use of an empty list is constrained to cons, which means
# it will transition to its cons-chain representation and
# become NULL.  Therefore, the implementation is recommended
# to create a VAR_CONS with the NULL value in these cases
# (each at the discretion of the implementation).
# However, a non-empty list shall be a cont?? on creation (such
# as from list, list-copy, reverse, make-list) and shall
# transition to a cons-chain on need such as dup or cdr-usage.
#
# In general, the same instance of [VAR_VOID] may be utilized
# to generate such a value, or a new instance may be
# created in any such occurance.  alias? is therefore
# unspecified for [VAR_VOID] in general.  However,
# for make-list all resulting elements contain one and the
# same instance.
#
# The effect of using splice inside a hard-quoted expression
# is undefined.  This is more of a known bug that is outside
# area of interest.  The observed behavior is undo of quote.
#
# String-comparison and whether a string may be populated
# with non utf8 are implementation-defined.  It is possible
# to allow feeding bytes onto an out-string and operate on
# the result -- not in the python impl. which internally
# uses semantic strings (not raw bytes).  A raw string
# implementation allowing non utf8 strings, may permit say
# a program processing EBCDIC from files or stdin and
# keeping buffers from this in string variables.  Such an
# impl. may provide string-bytes-ref and string-bytes-length
# similar to string-ref and string-length, but for raw byte
# access.  However, language-defined functions such as
# string->list shall operate with utf8 and fail otherwise.
#
# Comparison of bare splices (non-flattened) is undefined,
# and better left as eq? as this is not the intended use of
# splice, '@'.  Splice is for (list @(list)) yield (list)
# Also, getting at a bare splice requires macro usage.
#
# It is recommended that (take) yields a cons chain iff
# the argument list has a cons chain representation.
# (take) is not applicable on non-list.
#
# The set of cxr-functions should use list-ref for "a"
# followed by a number of "d"s (such as in "caddr"), so to
# avoid unneeded cdr-use (and thereby conversion to cons
# chain).  This is at discression of the implementation.
#
# Note that the "record-type" name is ignored completely,
# and only part of the syntax for r7rs compatibility.
#
# As an optimization the implementation should (maybe as
# part of compx) strip away non-last #void as well as all
# flattening LEX_SEQ diminishing need to eval it.
#


##
# code-points
##

# scan-tokens

LEX_BEG        = 1
LEX_END        = 2
LEX_DOT        = 3
LEX_QT         = 4
LEX_QQ         = 5
LEX_UNQ        = 6
LEX_SPL        = 7
LEX_R          = 8

name_cs = "!$%&*+-./:<=>?@^_~"
par_beg = "([{"  #| respective to
par_end = ")]}"  #| - closing.
quotes  = "'`,"  #| - their LEX_ codes.

# syntax tree

LEX_SYM        = 15  #| expanded quoted name
LEX_VOID       = 16
LEX_NUM        = 17
LEX_BOOL       = 18
LEX_STRING     = 19
LEX_NAM        = 20
LEX_REC        = 21
# no DICT      = 22
LEX_QUOTE      = 23
LEX_QUASIQUOTE = 24
LEX_UNQUOTE    = 25
# no SPLICE      26
# no APPLY       27
# no PORT        28
# no EOF         29
# LEX_OP not in py; instead OP_x directly
# no EXTRA MIN..MAX
NO_MASK        = 0b1111111
# no CONS        (1 << 7)
LEX_LIST       = (1 << 8)
LEX_NONLIST    = (1 << 9)
# no FUN         (1 << 10)
# no FUN_DOT     (1 << 11)
# no FUN_HOST    (1 << 12)

# run-types

BIT_VAR        = (1 << 14)
# may be set to zero, but for development
# handy to distinguish these values from
# the LEX entities.  this implementation
# uses the alignment of these values in
# conversion (i-e macro args and return)

# offsets with LEX_
# no SYM         15
VAR_VOID       = 16 + BIT_VAR
VAR_NUM        = 17 + BIT_VAR
VAR_BOOL       = 18 + BIT_VAR
VAR_STRING     = 19 + BIT_VAR
VAR_NAM        = 20 + BIT_VAR
VAR_REC        = 21 + BIT_VAR
VAR_DICT       = 22 + BIT_VAR
VAR_QUOTE      = 23 + BIT_VAR
# no QUASIQUOTE  24
VAR_UNQUOTE    = 25 + BIT_VAR
VAR_SPLICE     = 26 + BIT_VAR
VAR_APPLY      = 27 + BIT_VAR
VAR_PORT       = 28 + BIT_VAR
VAR_EOF        = 29 + BIT_VAR
# no OP in py
VAR_EXTRA_MIN  = 32 + BIT_VAR
VAR_EXTRA_MAX  = 127 + BIT_VAR
VAR_CONS       = (1 << 7) + BIT_VAR
VAR_LIST       = (1 << 8) + BIT_VAR
VAR_NONLIST    = (1 << 9) + BIT_VAR
VAR_FUN_OPS    = (1 << 10) + BIT_VAR
VAR_FUN_HOST   = (1 << 11) + BIT_VAR

# interpreter ops

#  APPLY when name (no op.code)
OP_BIND       = 1
OP_COND       = 2
OP_LAMBDA     = 3
OP_LAMBDA_DOT = 4
OP_SEQ        = 5
OP_IMPORT     = 6
OP_EXPORT     = 7

# known names

NAM_THEN       = 0  #| identifiers pre-interned as syntax but also
NAM_ELSE       = 1  #| to re-use for local names in a few macros
NAM_QUOTE      = 2      #| quote-processing are performed by macros
NAM_QUASIQUOTE = 3      #| on names
NAM_UNQUOTE    = 4      #| .
NAM_MACRO      = 5   #| the macro macro; defines a user-macro
NAM_CAR        = 6   #| names of functions used in some of the
NAM_EQVP       = 7   #| language-macros
NAM_LIST       = 8   #| used for conversions of macro-argument
NAM_NONLIST    = 9   #| into data-variable representation.
NAM_SETJJ      = 10  #| letrec, letrecx
NAM_DUP        = 11  #| define
NAM_ERROR      = 12  #| default else in case
NAM_SPLICE     = 13  #| @ expansion
NAM_IMPORT     = 14

def xn(n):
    return (LEX_NAM, n)

nam_then = xn(NAM_THEN)
nam_else = xn(NAM_ELSE)
nam_quote = xn(NAM_QUOTE)
nam_quasiquote = xn(NAM_QUASIQUOTE)
nam_unquote = xn(NAM_UNQUOTE)
nam_macro = xn(NAM_MACRO)
nam_car = xn(NAM_CAR)
nam_eqvp = xn(NAM_EQVP)
nam_list = xn(NAM_LIST)
nam_nonlist = xn(NAM_NONLIST)
nam_setjj = xn(NAM_SETJJ)
nam_dup = xn(NAM_DUP)
nam_error = xn(NAM_ERROR)
nam_splice = xn(NAM_SPLICE)
nam_import = xn(NAM_IMPORT)

##
# diagnostics
##

class SrcError(Exception):

    def __init__(self, message):
      super().__init__(message)

class RunError(Exception):

    def __init__(self, message):
      super().__init__(message)

def put(f, a):
    f.write(" ".join([str(x) for x in a]) + "\n")
    f.flush()

verbose = False
def set_verbose(v):
    global verbose
    verbose = v

import sys

def debug(*args):
    if verbose:
        put(sys.stdout, ["debug:"] + list(args))

def result(*args):
    put(sys.stdout, ["; ==>"] + list(args))

def warning(*args):
    put(sys.stdout, ["warning:"] + list(args))

def error(*args):
    put(sys.stderr, ["error:"] + list(args))
    sys.exit(1)

def broken(*args):
    raise RunError("broken: %r" % (args,))

##
# tokenization
##

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
            while i != n and s[i] != '\n':
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
                    raise SrcError("#| comment not ended")
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
    if s[i] in (par_beg + par_end):
        return s[i], i + 1
    w = i
    if s[i] == "#":
        i += 1
        if i == n:
            raise SrcError("stop at #")
        if s[i] == "\\":
            i += 1
            if i == n:
                raise SrcError("stop at #\\")
        if s[i].isspace():
            raise SrcError(s[w : i] + " space")
        if not s[i].isalnum():
            i += 1
        else:
            while True:
                i += 1
                if i == n or not s[i].isalnum():
                    break
        return s[w : i], i
    if s[i] == "\"":
        while i < n:
            i += 1
            if i == n:
                raise SrcError("stop in string")
            if s[i] == "\"":
                i += 1
                return s[w : i], i
            if s[i] == "\\":
                i += 1
                if i == n:
                    raise SrcError("stop in string at '\\'")
    if s[i] == "@":
        i += 1
        if i == n:
            raise SrcError("stop at @")
        return s[w : i], i
    if s[i] in quotes:
        i += 1
        if i == n:
            raise SrcError("stop at quote")
        return s[w : i], i
    while s[i].isalnum() or s[i] in name_cs:
        i += 1
        if i == n:
            break
    if i != w:
        return s[w : i], i
    raise SrcError("character '%c'" %(s[i],))

def unescape_string(s):
    octals = "01234567"
    a = []
    i = 0
    n = len(s)
    while i < n:
        c = s[i]
        if c == "\\":
            i += 1
            if i == n:
                broken("string ends at \\")
            c = s[i]
            if c == "t":
                c = "\t"
            elif c == "n":
                c = "\n"
            elif c == "r":
                c = "\r"
            elif c == '"':
                c = '"'
            elif c in octals:
                b = int(c)
                while i + 1 != n:
                    c = s[i + 1]
                    if c not in octals:
                        break;
                    i += 1
                    b *= 8
                    b += int(c)
                    if b > 255:
                        raise SrcError("octal overflow")
                c = chr(b)
            else:
                raise SrcError("string escape '%c'" % (c,))
        a.append(c)
        i += 1
    return "".join(a)

class Names:

    def __init__(self, *a):
        self.v = []
        self.m = {}
        for i, name in a:
            j = self._add(name)
            assert i == j

    def __len__(self):
        return len(self.v)

    def __getitem__(self, i):
        return self.v[i]

    def intern(self, name):
        if name in self.m:
            return self.m[name]
        return self._add(name)

    def _add(self, name):
        i = len(self.v)
        self.v.append(name)
        self.m[name] = i
        return i

def lex(s, names):
    n = len(s)
    i = 0
    if s.startswith("#!"):
        try:
            i = s.index('\n')
        except ValueError:
            raise SrcError("'#!' without end")
    a = []
    while i != n:
        t, i = tok(s, i)
        if not t:
            break
        v = None
        if t[0] in par_beg:
            v = (LEX_BEG, par_beg.index(t))
        elif t[0] in par_end:
            v = (LEX_END, par_end.index(t), linenumber)
        elif t[0].isdigit() or (len(t) != 1 and t[0] in "-+"):
            v = (LEX_NUM, int(t, 0))
        elif t[0] == "#":
            if t[1] in "tf":
                if len(t) == 2 or t[1:] in ("true", "false"):
                    v = (LEX_BOOL, "t" == t[1])
                else:
                    raise SrcError("# f or t")
            elif t[1] in "bodx":
                base = [2, 8, 10, 16]["bodx".index(t[1])]
                try:
                    v = (LEX_NUM, int(t[2:], base))
                except ValueError:
                    raise SrcError("# numeric")
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
                        raise SrcError("#\\ token")
            elif t == "#void":
                v = (LEX_VOID,)
            elif t == "#r":
                v = (LEX_R,)
            else:
                raise SrcError("# token")
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
            h = names.intern(t)
            v = (LEX_NAM, h, linenumber)
        a.append(v)
    debug("lex", a)
    return a

##
# parser
##

def lex_in(k, a):
    assert (k & BIT_VAR) == 0
    assert (a & NO_MASK) == 0
    return bool(k & a)

PARSE_MODE_TOP = -2  # inside no form paren
PARSE_MODE_ONE = -1  # yield single element
# otherwise 0..2:  par_beg most recent open
def parse_r(z, i, paren_mode, d):
    n = len(z)
    r = []
    while i != n:
        x = z[i]
        c = x[0]
        if c == LEX_END:
            if x[1] != paren_mode:
                raise SrcError("parens '%s' at line %d does not match '%s'" % (
                    par_end[x[1]],
                    x[2], par_beg[paren_mode] if paren_mode >= 0 else '(none)'))
            return r, i + 1
        elif c == LEX_BEG:
            s, i = parse_r(z, i + 1, x[1], d + 1)
            r.append(s)
        elif c in (LEX_QT, LEX_QQ, LEX_UNQ, LEX_SPL, LEX_R):
            x, i = parse_r(z, i + 1, PARSE_MODE_ONE, d)
            assert len(x) == 1
            if c == LEX_QT:
                x = [nam_quote, *x]
            elif c == LEX_QQ:
                x = [nam_quasiquote, *x]
            elif c == LEX_UNQ:
                x = [nam_unquote, *x]
            elif c == LEX_SPL:
                x = [nam_splice, *x]
            elif c == LEX_R:
                if type(x) != list:
                    raise SrcError("#r takes form")
                x = (LEX_REC, [quote(y, False) for y in x[0]])
            else:
                broken("%d unexpected" % (c,))
            r.append(x)
        else:
            i += 1
            r.append(x)
        if paren_mode == PARSE_MODE_ONE:
            debug("parse1", r)
            return r, i
    if paren_mode != PARSE_MODE_TOP:
        raise SrcError("parens '%s' depth %d not closed" % (
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
    # Note:  I could let a Macro class hierarchy take care of
    # cases for user and quote recursion control so that
    # conditions are not stated here. That is a choice.
    if type(t) != list or len(t) == 0:
        return t
    is_user = is_quote = current = False
    is_macro = t[0][0] == LEX_NAM and t[0][1] in macros
    if is_macro:
        is_user = isinstance(macros[t[0][1]], UserMacro)
        current = qq == 0
        if t[0][1] == NAM_QUOTE:
            is_quote = True
        elif t[0][1] == NAM_QUASIQUOTE:
            qq += 1
        elif t[0][1] == NAM_UNQUOTE:
            qq -= 1
            if qq == 0:
                current = True
    if is_user:
        # A user-macro must observe input prior to language-macros
        # expansion, which is instead performed below, as their
        # output is not interpreter-ready as for language-macros.
        # however, the quotation macros are processed on the args
        # in addition to eventually on their output.
        args_exp = { NAM_QUOTE: macros[NAM_QUOTE],
                NAM_QUASIQUOTE: macros[NAM_QUASIQUOTE],
                NAM_UNQUOTE: macros[NAM_UNQUOTE] }
    elif is_quote:
        # Normal quote arguments must not know of macros except
        # for quasi-quotation processing.
        args_exp = { NAM_QUASIQUOTE: macros[NAM_QUASIQUOTE],
                NAM_UNQUOTE: macros[NAM_UNQUOTE] }
    else:
        args_exp = macros
    for i in range(len(t)):
        t[i] = expand_macros(t[i], args_exp, qq)
    if current:
        t = macros[t[0][1]](t)
        if is_user:
            t = expand_macros(t, macros, qq)
    debug("expand", t)
    return t

def readx(s, names):
    global linenumber
    linenumber = 1
    try:
        z = lex(s, names)
    except SrcError as e:
        raise SrcError("line %d: " % (linenumber,) + e.args[0])
    t, i = parse_r(z, 0, PARSE_MODE_TOP, 0)
    if i != len(z):
        broken("not fully consumed; unexpected")
    # debug("read", t)
    return t

def parse(s, names, macros):
    t = readx(s, names)
    try:
        expand_macros(t, macros, 0)
    except KeyError as e:
        i = e.args[0]
        if i < len(names):
            raise SrcError("%s (%d) unbound in macro-expand" % (names[i], i))
        raise CoreError("parse internal key error: %d" % (i,))
    except IndexError as e:
        raise SrcError("form syntax error")
    # debug("tree", t)
    return t

def unbound(t, defs, is_block):
    # Capture of free names are done in macros themselves such as
    # let and lambda, by invoking this function.  Note that the
    # identifiers has not yet been "zloc" processed.
    r = set()
    from_branches = set()
    for x in t:
        if type(x) != list:
            if x[0] == LEX_NAM and x[1] not in defs:
                r.add(x[1])
            elif lex_in(x[0], LEX_LIST | LEX_NONLIST):
                r.update(unbound(x[1], defs, False))
        elif type(x[0]) != int:
            r.update(unbound(x, defs, False))
        elif x[0] == OP_BIND:
            if not is_block:
                raise SrcError("define in non-block")
            i = x[1]
            u = unbound([x[2]], defs, False)
            if i in defs:
                debug("re-define")
            r.update(u)
            defs.add(x[1])
        elif x[0] in (OP_LAMBDA, OP_LAMBDA_DOT):
            from_branches.update(x[2])
        elif x[0] == OP_COND:
            r.update(unbound(x[1:], defs, False))
        elif x[0] == OP_SEQ:
            for y in x[1:]:
                r.update(unbound([y], defs, True))
        elif x[0] == OP_IMPORT:
            defs.update(x[1])
        elif x[0] == OP_EXPORT:
            pass
        else:
            broken("unknown form")

    from_branches.difference_update(defs)
    r.update(from_branches)
    return r

def find_unbound(t, y):
    for x in t:
        if type(x) != list:
            if x[0] == LEX_NAM:
                if x[1] == y:
                    return x
            elif lex_in(x[0], LEX_LIST | LEX_NONLIST):
                r = find_unbound(x[1], y)
                if r:
                    return r
        elif type(x[0]) != int:
            r = find_unbound(x, y)
            if r:
                return r
        elif x[0] == OP_BIND:
            r = find_unbound([x[2]], y)
            if r:
                return r
        elif x[0] in (OP_LAMBDA, OP_LAMBDA_DOT):
            if y in x[2]:
                return find_unbound(x[3:], y)
        elif x[0] in (OP_COND, OP_SEQ):
            r = find_unbound(x[1:], y)
            if r:
                return r
        elif x[0] in (OP_IMPORT, OP_EXPORT):
            pass
        else:
            broken("unknown form")

def report_unbound(u, t, names):
    if not u:
        return
    def info(x, names):
        a = []
        if x[0] != LEX_NAM:
            broken("(non-name)")
        elif len(x) == 3:
            a.append("line %d: " % (x[2]))
        a.append("%s" % (names[x[1]],))
        return "".join(a)
    a = []
    for y in u:
        x = find_unbound(t, y)
        if x:
            a.append(info(x, names))
        else:
            a.append("(reportedly) %s" % (names[y],))
    raise SrcError("unbound,\n" + "\n".join(a))

def compx(s, names, macros, env_keys):
    # "compile" or "compl" would give clashes
    t = parse(s, names, macros)
    u = unbound(t, set(env_keys), True)
    report_unbound(u, t, names)
    zloc_scopes(t, None)
    return t

class LexEnv:

    def __init__(self, parms, capture):
        self.n_parms = len(parms)
        self.names = parms + capture
        self.n_init = len(self.names)

    def __repr__(self):
        return "<%r %r>" % (self.names[:self.n_parms],
                self.names[self.n_parms:])

    def rewrite_name(self, n):
        try:
            return self.names.index(n)
        except ValueError:
            pass
        i = len(self.names)
        self.names.append(n)
        return i

    def rewrite_names(self, c):
        assert type(c) == list
        r = []
        for n in c:
            r.append(self.rewrite_name(n))
        return r

    def activation(self, captured, dot, args):
        env = [None] * len(self.names)
        for i, v in enumerate(captured):
            env[i + self.n_parms] = v
        if dot:
            last = self.n_parms - 1
            if len(args) < last:
                raise RunError("fun-dot expected %d args got %d"
                        % (self.n_parms, len(args)))
            for i in range(last):
                env[i] = args[i]
            env[last] = [VAR_LIST, list(args[last:])]
        else:
            if len(args) != self.n_parms:
                raise RunError("fun expected %d args got %d"
                        % (self.n_parms, len(args)))
            for i, v in enumerate(args):
                env[i] = v
        return env

# Function does "zip-locate" (compact) the identifiers in a lexical
# scope (lambda) to use monotonically increasing indexes as keys.
# The name identifiers in code are rewritten for their scopes
# LexEnv that in this manner handles just those names.
def zloc_scopes(t, local_env):
    for i, x in enumerate(t):
        if type(x) != list:
            if x[0] == LEX_NAM:
                if local_env:
                    t[i] = (x[0], local_env.rewrite_name(x[1]))
            elif lex_in(x[0], LEX_LIST | LEX_NONLIST):
                t[i] = (x[0], zloc_scopes(x[1], local_env))
        elif type(x[0]) != int:
            t[i] = zloc_scopes(x, local_env)
        elif x[0] == OP_BIND:
            if local_env:
                x[1] = local_env.rewrite_name(x[1])
            x[2:] = zloc_scopes([x[2]], local_env)
        elif x[0] in (OP_LAMBDA, OP_LAMBDA_DOT):
            c = sorted(x[2])
            fun_env = LexEnv(x[1], c)
            x[3:] = zloc_scopes(x[3:], fun_env)
            x[1] = fun_env
            if local_env:
                c = local_env.rewrite_names(c)
            x[2] = c
        elif x[0] in (OP_COND, OP_SEQ):
            x[1:] = zloc_scopes(x[1:], local_env)
        elif x[0] == OP_IMPORT:
            # an import has already parsed (zloc performed),
            # and I therefore in m_scope also zloc.
            pass
        elif x[0] == OP_EXPORT:
            pass
        else:
            broken("unknown form")
    return t

##
# builtin macros
##

def mchk_or_fail(c, message):
    if not c:
        raise SrcError(message)

def margc_must_chk(c, mn, x, z):
    mchk_or_fail(c, "%s requires %d args but got %d"
            % (mn, z - 1, x - 1))

def margc_must_eq(mn, s, z):
    margc_must_chk(len(s) == z, mn, len(s), z)

def margc_must_ge(mn, s, z):
    margc_must_chk(len(s) >= z, mn, len(s), z)

# naming - let

def m_ref(s):
    s[0] = OP_BIND
    if type(s[1]) == list:
        n = s[1][0]
        s[2] = m_lambda([-99, s[1][1:], *s[2:]])
        s[1] = n
        s = m_ref(s)
    else:
        mchk_or_fail(s[1][0] == LEX_NAM, "ref.1 expects name")
        s[1] = s[1][1]
    i = s[1]
    d = s[2]
    u = unbound([d], set(), False)
    if i in u:
        # permit define of recursive lambda by wrapping with
        # a letrec.  this leaves immediate i-e (ref i (+ i))
        # "non-working", then instead use define
        y = (LEX_NAM, i)
        return [OP_BIND, i,
                m_letrec([-99, [[y, d]], y])]
    return s

def m_define(s):
    if type(s[1]) == list:
        return m_ref(s)
    s[0] = OP_BIND
    mchk_or_fail(s[1][0] == LEX_NAM, "define.1 expects name")
    s[1] = s[1][1]
    d = s[2]
    s[2] = [nam_dup, d]
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

def bnd_unzip(s):
    if type(s) != list:
        raise SrcError("let expected sub-form")
    a = []
    v = []
    for z in s:
        if type(z) != list:
            raise SrcError("let expected binding")
        x, y = z
        if x[0] != LEX_NAM:
            raise SrcError("let binding not to name")
        a.append(x[1])
        v.append(y)
    return a, v

def recset(let):
    a = let[0][1]
    u = let[0][2]
    b = []
    for z in a:
        assert z < 0
        n = -z
        b.append(n)
        let[0].insert(3, [nam_setjj, (LEX_NAM, n), (LEX_NAM, z)])
    u.add(NAM_SETJJ)
    u.update(b)
    u.update(unbound(let[1:], a, False))

def rectmp(let, a):
    u = let[0][2]
    v = []
    for z in a:
        assert z >= 0
        v.append((LEX_VOID,))
    w = set(u)
    w.difference_update(a)
    return [[OP_LAMBDA, a, w, let], *v]

def m_let(s, rec=False):
    margc_must_ge("let", s, 2)
    if type(s[1]) != list and s[1][0] == LEX_NAM:
        return named_let(s)
    lbd = [OP_LAMBDA]
    a, v = bnd_unzip(s[1])
    if rec:
        t = a
        a = [-z for z in t]
    lbd.append(a)
    lbd.append(unbound(s[2:], set(a), True))
    if len(s) == 2:
        lbd.append((LEX_VOID,))
    else:
        lbd.extend(s[2:])
    r = [lbd, *v]
    if rec:
        recset(r)
        r = rectmp(r, t)
    return r

def m_letx(s, rec=False):
    margc_must_ge("let*", s, 2)
    block = s[2:]
    if not block:
        block = [(LEX_VOID,)]
    mchk_or_fail(type(s[1]) == list, "let* expected sub-form")
    if not s[1]:
        return [[OP_LAMBDA, [], unbound(block, set(), True), *block]]
    t = []
    for z in reversed(s[1]):
        lbd = [OP_LAMBDA]
        x, y = z  # lambda wrap each as opposed to let's unzip
        mchk_or_fail(x[0] == LEX_NAM, "let* binding not to name")
        a = [x[1]]
        if rec:
            t.extend(a)
            a = [-a[0]]
        lbd.append(a)
        lbd.append(unbound(block, set(a), True))
        lbd.extend(block)
        r = [lbd, y]
        if rec:
            recset(r)
        block = [r]
    r = block[0]
    if rec:
        r = rectmp(r, t)
    return r

def m_letrec(s):
    return m_let(s, True)

def m_letrecx(s):
    return m_letx(s, True)

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
    if type(parms) != list:
        raise SrcError("do parameters")
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
    # TBD:  look into optimization and allowance, such as here
    # length 1: return Void, length 2: just return s[1] itself
    return m_letx([-99, [], *s[1:]])

# quotation

def quote(v, quasi):
    if type(v) == list:
        if is_dotform(v):
            w = [quote(x, quasi) for x in without_dot(v)]
            if type(w[-1]) == tuple \
                    and lex_in(w[-1][0], LEX_LIST | LEX_NONLIST):
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
            broken("scanner-token")
            return (LEX_NAM, v[1])
        if v[0] == LEX_UNQUOTE:
            return v[1]
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
        if v[0] == LEX_UNQ:
            broken("scan token")
        if v[0] in (LEX_NAM, LEX_UNQUOTE):
            return (LEX_UNQUOTE, v)
        if v[0] == LEX_SYM:
            return (LEX_NAM, v[1])
        return v
    return unquote(s[1])

# macro

i_macros = {}

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
        return from_lex([nam_quote, (LEX_NAM, s[1])])
    if s[0] == LEX_VOID:
        return [VAR_VOID,]
    if s[0] == LEX_REC:
        v = [xeval(y, None) for y in s[1]]
        if v[0][0] != VAR_NAM:
            raise SrcError("record-id not name")
        return [VAR_REC, Record(v[0][1], v[1:])]
    if s[0] not in (LEX_NAM, LEX_NUM, LEX_BOOL, LEX_STRING,
            LEX_QUOTE, LEX_UNQUOTE):
        raise SrcError("from lex %r" % (s,))
    if s[0] == LEX_QUOTE:
        # try to go away from having VAR_QUOTE at all
        # should not be possible.  leave VAR_UNQUOTE
        # as macro expand leaves it if bare
        broken("quote from lex")
    return [s[0] + BIT_VAR, s[1]]

def to_lex(s):
    if s[0] == VAR_CONS:
        s = s[1].to_list_var()
    if s[0] in (VAR_LIST, VAR_NONLIST):
        r = [to_lex(x) for x in s[1]]
        return with_dot(r) if s[0] == VAR_NONLIST else r
    if s[0] == VAR_REC:
        r = [quote(to_lex(x), False) for x in (s[1].values)]
        return (LEX_REC, [(LEX_SYM, s[1].nam), *r])
    if s[0] == VAR_VOID:
        return (LEX_VOID,)
    if s[0] not in (VAR_NAM, VAR_NUM, VAR_BOOL, VAR_STRING,
            VAR_QUOTE, VAR_UNQUOTE):
        raise SrcError("to lex %r" % (s,))
    return (s[0] - BIT_VAR, s[1])

class UserMacro:

    def __init__(self, mname, parms, isdot, block, names):
        self.mname = mname
        self.parms = parms
        self.isdot = isdot
        self.block = block
        self.names = names

    def __call__(self, t):
        args = [from_lex(x) for x in t[1:]]
        debug("user-macro args", args)
        env = Overlay(i_env)
        if self.isdot:
            last = len(self.parms) - 1
            margc_must_chk(len(args) >= last, self.mname,
                    len(args) + 1, last + 1)
            for i in range(last):
                env[self.parms[i]] = args[i]
            env[self.parms[last]] = [VAR_LIST, args[last:]]
        else:
            margc_must_chk(len(args) == len(self.parms), self.mname,
                    len(args) + 1, len(self.parms) + 1)
            for i, v in enumerate(args):
                env[self.parms[i]] = v
        for x in self.block:
            r = run(x, env)
        debug("user-macro result", r)
        r = to_lex(r)
        debug("result as lex", r)
        return r

def m_macro(names, macros):
    def macro(s):
        margc_must_ge("macro", s, 4)
        if s[1][0] != LEX_NAM:
            raise SrcError("non-name macro")
        n = s[1][1]
        y = s[2]
        if type(y) != list:
            isdot = True
            y = [y]
        else:
            isdot = is_dotform(y)
            if isdot:
                y = without_dot(y)
        block = s[3:]
        zloc_scopes(block, None)
        parms = []
        for p in y:
            mchk_or_fail(p[0] == LEX_NAM, "macro params must be names")
            parms.append(p[1])
        macros[n] = UserMacro(names[n], parms, isdot, block, names)
        return (LEX_VOID,)
    return macro

def m_gensym(names):
    def gensym(s):
        i = len(names)
        j = names.intern("&%d" % (i,))
        assert j == i
        return (LEX_SYM, i)
    return gensym

def m_seq(s):
    # handly to impact scope with macro expr (one)
    s[0] = OP_SEQ
    return s

# bare lex, skipping linenumber info
# eases comparison
def blex(y):
    return y[:2]

# conditions

def m_cond(s):
    s[0] = OP_COND
    n = len(s)
    i = 1
    while i < n:
        if blex(s[i][0]) == nam_else:
            return s[:i] + [[(LEX_BOOL, True),
                m_begin([-99, *s[i][1:]])]]
        if blex(s[i][1]) == nam_then:
            break
        if len(s[i]) != 2:
            s[i] = [s[i][0], m_begin([-99, *s[i][1:]])]
        i += 1
    if i == n:
        return s
    return s[:i] + [[(LEX_BOOL, True),
        m_let([-99, [[nam_then, s[i][0]]],
            m_cond([-99, [nam_then, [s[i][2], nam_then]], *s[i + 1:]])])]]

def m_if(s):
    if len(s) == 3:
        s.append((LEX_VOID,))
    elif len(s) != 4:
        raise SrcError("if-expression of length %d" % (len(s),))
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

def xcase_test(t):
    if type(t) != list:
        if blex(t) != nam_else:
            raise SrcError("case neither form nor else")
        return (LEX_BOOL, True)
    return m_or([-99, *[[nam_eqvp, v, nam_else] for v in t]])

def xcase_target(t):
    if blex(t[0]) != nam_then:
        if len(t) != 1:
            raise SrcError("length %d case target" % (len(t),))
        return t[0]
    if len(t) != 2:
        raise SrcError("length %d case => target" % (len(t),))
    return [t[1], nam_else]

def xcase(s):
    if blex(s[-1][0]) != nam_else:
        s.append([nam_else, nam_then, nam_error])
    # deviation: from r7rs which states that result is unspecified
    # when no cases match and no "else".  if not using this result
    # in such a situation there would be no ill-effect.
    # rationale: hard to find issues may arise if returning the
    # VOID value, so early detection saves this problem that is
    # then addressed by programming a propper else-case.
    # alt: instead have [nam_else, (LEX_VOID,)] above.
    return m_or([-99, *[
        m_and([-99, xcase_test(ce[0]),
            xcase_target(ce[1:])
            if blex(ce[0]) != nam_else or blex(ce[1]) == nam_then
            else m_begin([-99, *ce[1:]])])
        for i, ce in enumerate(s)]])

def m_case(s):
    # note: abuse of "else" as switch variable name
    return m_letx([-99, [[nam_else, s[1]]], xcase(s[2:])])

# (here-) import

class Overlay:

    def __init__(self, d):
        assert type(d) == dict
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

def m_scope(names, env_keys):
    def xscope(s):
        # a.k.a "here-import"
        set_up = dict()
        if s[1][0] != OP_EXPORT:
            raise SrcError("missing export")
        for n in s[1][1:]:
            if n[0] != LEX_NAM:
                raise SrcError("export of non-name")
            y = n[1]
            set_up[y] = y
        t = s[2:]
        u = unbound(t, set(env_keys), True)
        report_unbound(u, t, names)
        return [OP_IMPORT, set_up, *zloc_scopes(t, None)]
    return xscope

def m_import(names, macros, opener):
    def ximport(s):
        u_fn = opener.filename
        mchk_or_fail(len(s) in (2, 3), "import expects 2 or 3 args")
        mchk_or_fail(s[1][0] == LEX_STRING, "import.1 expects string")
        e_macros = dict(i_macros)
        e_macros[NAM_MACRO] = m_macro(names, e_macros)
        e_macros[NAM_IMPORT] = m_import(names, e_macros, opener)
        try:
            f = opener(s[1][1])
        except:
            raise SrcError("no such file")
        global linenumber
        u_ln = linenumber
        r = compx(f.read(), names, e_macros, i_env.keys())
        f.close()
        prefix_s = None
        if len(s) == 3:
            p = s[2]
            if p[0] not in (LEX_NAM, LEX_SYM):
                raise SrcError("non-name import-prefix")
            prefix_s = names[p[1]]
        set_up = dict()
        if r[0][0] != OP_EXPORT:
            raise SrcError("missing export")
        for n in r[0][1:]:
            x = y = n[1]
            if n[0] == LEX_NAM:
                if prefix_s:
                    y = names.intern(prefix_s + names[x])
                set_up[y] = x
            elif n[0] == LEX_SYM:
                if prefix_s and p[0] == LEX_SYM:
                    y = names.intern(prefix_s + names[x])
                if x not in e_macros:
                    raise SrcError("no macro %s"
                            % (names[x],))
                macros[y] = e_macros[x]
            else:
                raise SrcError("export of non-name")
        # note: unbound check with i_env not needed, as this will
        #       be done from top-level
        opener.filename = u_fn
        linenumber = u_ln
        return [OP_IMPORT, set_up, *r[1:]]
    return ximport

def m_export(s):
    s[0] = OP_EXPORT
    return s

##
# builtin functions
##

#
# The arguments to a functions is passed by a list of
# object-references.  These objects are the lvalues as
# may be referred to by variable-names.  I call these
# objects "variables".
#
# Notes on Variables
#
# A variable is the value of an entry in the environment,
# and/or held by another structure such as a list.
# In this implementation we represent it with a mutable
# pair [type, value].
#
# No function operates on the environment.  Our set! operates
# on the variable.  set!! is needed to change its type and
# avoid any warning that may be enabled for set!
# list-set! replaces the list member and does not affect
# any variable as such.  special forms such as
# DEFINE (i-e ref) operate on the environment.
#
# A new non-empty list variable is represented by a
# contiguous container.  When another reference to
# it (th list, not a variable held) at any
# index is given out (either by "cdr-usage" or by
# "dup" of the list so that another variable holds it)
# requires it to convert to a cons-chain, so that
# any observation of mutation (such as inserting an
# element) is shared.  Note that it is a choice to
# not have a "shared-list" type that would avoid the
# need to transform the representation on "dup".
#
# The contigous representation of a cons chain as mentioned,
# uses a specific type to denote if it represents a normal
# list, or that it represents a non-list; a cons-chain
# where the last CDR is not NULL, by type respectively
# VAR_LIST and VAR_NONLIST.
#
# The value of a CONS variable is a cons-cell, while the
# value of a (NON)LIST variable is the contigous list as
# a whole and owned privately by this variable.
# The latter list value is not shared by other variables
# (but the variable itself may be refered to by several
# environments).  A cons-cell, on the other hand, may be.
#
# DICT is an explicit type for efficient lookup operations
# and is converted from an alist or back, VAR_DICT.  such a
# variable is not eqv? itself.
#
# The record type VAR_REC is not meant for a program but used
# by the define-record-type macro.
#
# VAR_STRING owns the value.  set! will require a complete
# copy of the value.  An implementation may introduce a
# (not visible to the language used) variable type for
# "big strings" that could instead own a shared pointer,
# to avoid such copy.  String values are immutable,
# so operations such as concatenation on strings, will
# copy memory to a significant degree anyway.
#
# VAR_NUM is trivial.  However, semantics may be surprising
# as one would expect any numerical expression to yield
# creation of a new instance:  a single number will *not*.
#
# VAR_VOID is used for "unspecified value" as in r7rs.
# object identity under alias? is not well defined,
# except for the result of make-list.
#

# support for CONS

def is_cons(y):
    return isinstance(y, Cons)

class Cons:

    # note: non-recursive iteration
    # improvement: opt.loop detect by mark of cons,
    #              w (i-e incr.gen) unique mark per loop-run

    # todo: pass last-holder option where used,
    #       instead of static last member.  in either
    #       implementation it may keep big memory,
    #       and the work around would be to clear it in
    #       various call-sites.  the suggested more explicit
    #       approach will be more manageable.

    class Iter:

        def __init__(self, c):
            self.c = c

        def __iter__(self):
            return self

        def __next__(self):
            c = self.c
            if c is None:
                raise StopIteration
            if not is_cons(c):
                broken("NONLIST")
            self.c = c.d
            return c.a

    def __init__(self, a, d):
        self.a = a  # CAR will be a VAR_xyz
        self.d = d  # CDR will be a Cons (not VAR_CONS) or None,
        #             or in case of a non-list will be a VAR_xyz

    def xcopy(self, n):
        """zero n means all"""
        r = c = Cons(self.a, None)
        for a in Cons.Iter(self.d):
            c.d = Cons(a, None)
            c = c.d
            n -= 1
            if n == 0:
                break
        else:
            Cons.last = c
        return r

    def length(self):
        r = 1
        for a in Cons.Iter(self.d):
            r += 1
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

    @staticmethod
    def from_list(x):
        if len(x) == 0:
            return None
        r = Cons(x[-1], None)
        Cons.last = r
        for e in reversed(x[:-1]):
            r = Cons(e, r)
        return r

    @staticmethod
    def from_nonlist(x):
        assert len(x) >= 2
        Cons.last = None
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
    if a is None:
        return []
    if is_cons(a):
        r = a.to_list_var()
        if r[0] != VAR_LIST:
            raise RunError("nonlist for list-use")
        return r[1]
    assert type(a) == list
    return a

def ConsOrListIter(c):
    if type(c) == list:
        return iter(c)
    return Cons.Iter(c)

# arg checks

def var_in(k, a):
    assert (k & BIT_VAR) != 0
    assert (a & NO_MASK) == 0
    return bool(k & a & ~BIT_VAR)

def var_members(a):
    r = []
    i = 1
    a &= ~BIT_VAR
    while a:
        if (a & i) != 0:
            r.append(i | BIT_VAR)
            a &= ~i
        i <<= 1
    return r

def var_type_name(vt):
    if vt >= VAR_EXTRA_MIN and vt <= VAR_EXTRA_MAX:
        return "extra-%x" % (~BIT_VAR & vt,)
    return { VAR_VOID: "void", VAR_CONS: "cons", VAR_LIST: "list",
        VAR_NONLIST: "nonlist", VAR_SPLICE: "splice", VAR_NUM: "number",
        VAR_BOOL: "boolean", VAR_STRING: "string", VAR_DICT: "dict",
        VAR_NAM: "name", VAR_QUOTE: "quote", VAR_UNQUOTE: "unquote",
        VAR_FUN_OPS: "fun", VAR_FUN_HOST: "fun-host",
        VAR_APPLY: "apply", VAR_PORT: "port", VAR_EOF: "eof-object" }[vt]

def fargt_repr(vt):
    try:
        return var_type_name(vt)
    except KeyError:
        return "[%d]" % (vt,)

def fchk_or_fail(c, message):
    if not c:
        raise RunError(message)

def fargs_count_fail(fn, k, n):
    raise RunError("%s requires %d args but got %d"
            % (fn, n, k))

def fargc_must_eq(fn, args, n):
    if len(args) != n:
        fargs_count_fail(fn, len(args), n)

def fargc_must_ge(fn, args, n):
    if len(args) < n:
        fargs_count_fail(fn, len(args), n)

def fargt_must_eq(fn, args, i, vt):
    if len(args) <= i:
        raise RunError("%s with no args[%d]" % (fn, i))
    if args[i][0] != vt:
        raise RunError("%s expected args[%d] %s got %s"
            % (fn, i,
                fargt_repr(vt), fargt_repr(args[i][0])))

def fargt_must_in(fn, args, i, vts):
    if not var_in(args[i][0], vts):
        raise RunError("%s args[%d] accepts %r got %s"
                % (fn, i, "/".join(fargt_repr(vt)
                    for vt in var_members(vts)),
                    fargt_repr(args[i][0])))

# functions on CONS, (NON)LIST

def f_list(*args):
    if len(args) == 0:
        return [VAR_CONS, None]
    return [VAR_LIST, list(args)]

def f_nonlist(*args):
    # note: not cat'ing as done on (nested) dots
    # in xeval -- i-e (0 . (1 2)) ==> (0 1 2)
    return [VAR_NONLIST, list(args)]

def f_list_copy(*args):
    fn = "list-copy"
    fargc_must_eq(fn, args, 1)
    fargt_must_in(fn, args, 0, VAR_LIST | VAR_CONS)
    return [VAR_LIST, normal_list(args[0][1])]

def f_cons(*args):
    fargc_must_eq("cons", args, 2)
    if var_in(args[1][0], VAR_CONS | VAR_LIST | VAR_NONLIST):
        c = to_cons(args[1])
        args[1][:] = [VAR_CONS, c]
        return [VAR_CONS, Cons(args[0], c)]
    return [VAR_NONLIST, list(args)]

def f_car(*args):
    fn = "car"
    fargc_must_eq(fn, args, 1)
    fargt_must_in(fn, args, 0, VAR_CONS | VAR_LIST | VAR_NONLIST)
    if var_in(args[0][0], VAR_LIST | VAR_NONLIST):
        return args[0][1][0]
    assert args[0][0] == VAR_CONS
    if not args[0][1]:
        raise RunError("car on null")
    if not is_cons(args[0][1]):
        broken("cons variable has non-cons")
    return args[0][1].a

def f_list_ref(*args):
    def c_list_ref(c, i):
        while i:
            c = c.d
            i -= 1
        return c.a
    fargt_must_eq("list-ref", args, 1, VAR_NUM)
    if args[0][0] == VAR_CONS:
        return c_list_ref(args[0][1], args[1][1])
    if not var_in(args[0][0], VAR_LIST | VAR_NONLIST):
        raise RunError("list-ref on %s"
                % (fargt_repr(args[0][0]),))
    return args[0][1][args[1][1]]

def f_cdr(*args):
    fn = "cdr"
    fargc_must_eq(fn, args, 1)
    fargt_must_in(fn, args, 0, VAR_CONS | VAR_LIST | VAR_NONLIST)
    t = args[0][0]
    a = args[0][1]
    if t != VAR_CONS:
        assert var_in(t, VAR_LIST | VAR_NONLIST)
        if t == VAR_NONLIST:
            if len(a) <= 1:
                broken("short nonlist")
            if len(a) == 2:
                return a[1]
        a = to_cons([t, a])
        args[0][:] = [VAR_CONS, a]
    if a is None:
        raise RunError("cdr on null")
    return [VAR_CONS, a.d]

def f_append(*args):
    if len(args) == 0:
        return [VAR_CONS, None]
    if len(args) == 1:
        return args[0]
    i_last = len(args) - 1
    last = args[i_last]
    if var_in(last[0], VAR_LIST | VAR_NONLIST):
        last[1] = to_cons(last)
        last[0] = VAR_CONS
    r = p = to_cons_copy(args[0])
    # print("arg[0]: ", args[0][0] - BIT_VAR, type(args[0][1]), id(args[0][1]))
    # print("r: ", type(r), id(r))
    q = None
    for i, c in enumerate(args):
        if i == 0:
            continue
        if p is not None:
            q = Cons.last
        if i == i_last:
            if last[0] == VAR_CONS:
                p = last[1]
            else:
                p = last
        else:
            fargt_must_in("append", args, i, VAR_CONS | VAR_LIST)
            p = to_cons_copy(c)
        if q is not None:
            q.d = p
    # print("R", id(r))
    # print("Q", id(q))
    # print("P type", type(p))
    # print("P", id(p))
    # print("D", id(r.d))
    return [VAR_CONS, r]

def f_set_carj(*args):
    fn = "set-car!"
    fargc_must_eq(fn, args, 2)
    fargt_must_in(fn, args, 0, VAR_CONS | VAR_LIST | VAR_NONLIST)
    if args[0][0] == VAR_CONS:
        args[0][1].a = args[1]
    else:
        args[0][1][0] = args[1]
    return [VAR_VOID]

def f_set_cdrj(*args):
    fn = "set-cdr!"
    fargc_must_eq(fn, args, 2)
    fargt_must_in(fn, args, 0, VAR_CONS | VAR_LIST | VAR_NONLIST)
    if var_in(args[1][0], VAR_LIST | VAR_NONLIST):
        args[1][1] = to_cons(args[1])
        args[1][0] = VAR_CONS
    if args[1][0] == VAR_CONS:
        d = args[1][1]
    else:
        d = args[1]
    if args[0][0] == VAR_CONS:
        args[0][1].d = d
        return [VAR_VOID]
    if var_in(args[0][0], VAR_LIST | VAR_NONLIST):
        a = args[0][1][0]
    else:
        a = args[0]
    args[0][1] = Cons(a, d)
    args[0][0] = VAR_CONS
    return [VAR_VOID]

def f_list_tail(*args):
    fn = "list-tail"
    fargc_must_eq(fn, args, 2)
    fargt_must_in(fn, args, 0, VAR_CONS | VAR_LIST)
    fargt_must_eq(fn, args, 1, VAR_NUM)
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
    fargc_must_eq(fn, args, 3)
    fargt_must_in(fn, args, 0, VAR_CONS | VAR_LIST)
    fargt_must_eq(fn, args, 1, VAR_NUM)
    n = args[1][1]
    if args[0][0] == VAR_LIST:
        args[0][1][n] = args[2]
    else:
        c = f_list_tail(*args[:2])
        assert c[0] == VAR_CONS
        if c[1] is not None:
            c[1].a = args[2]
    return [VAR_VOID]

def f_make_list(*args):
    fn = "make-list"
    fargc_must_ge(fn, args, 1)
    fargt_must_eq(fn, args, 0, VAR_NUM)
    x = [VAR_VOID] if len(args) == 1 else args[1]
    n = args[0][1]
    return [VAR_LIST, [x] * n]

def f_reverse(*args):
    fargt_must_in("reverse", args, 0, VAR_CONS | VAR_LIST)
    r = f_list_copy(*args)
    r[1].reverse()
    return r

def f_take(*args):
    fn = "take"
    fargc_must_eq(fn, args, 2)
    fargt_must_eq(fn, args, 0, VAR_NUM)
    fargt_must_in(fn, args, 1, VAR_CONS | VAR_LIST)
    n = args[0][1]
    if args[1][0] == VAR_LIST:
        return [VAR_LIST, args[1][1][:n]]
    if n == 0 or args[1][1] is None:
        return [VAR_CONS, None]
    return args[1][1].xcopy(n)

def f_splice(*args):
    fn = "splice"
    fargc_must_eq(fn, args, 1)
    fargt_must_in(fn, args, 0, VAR_CONS | VAR_LIST)
    return [VAR_SPLICE, normal_list(args[0][1])]

# boolean functions (one, others are macros on cond)

def f_not(*args):
    fargc_must_eq("not", args, 1)
    return [VAR_BOOL, args[0] == [VAR_BOOL, False]]

# functions on NUM

def f_pluss(*args):
    r = 0
    for i, x in enumerate(args):
        fargt_must_eq("+", args, i, VAR_NUM)
        r += x[1]
    return [VAR_NUM, r]

def f_minus(*args):
    fargt_must_eq("-", args, 0, VAR_NUM)
    if len(args) == 1:
        return [VAR_NUM, -args[0][1]]
    r = args[0][1]
    for i, x in enumerate(args[1:], 1):
        fargt_must_eq("-", args, i, VAR_NUM)
        r -= x[1]
    return [VAR_NUM, r]

def f_multiply(*args):
    r = 1
    for i, x in enumerate(args):
        fargt_must_eq("*", args, i, VAR_NUM)
        r *= x[1]
    return [VAR_NUM, r]

def rdiv(args, fn):
    fargt_must_eq(fn, args, 0, VAR_NUM)
    n = args[0][1]
    d = 1
    for i, x in enumerate(args[1:], 1):
        fargt_must_eq(fn, args, i, VAR_NUM)
        d *= x[1]
        if d > n * 2:
            break
    return n, d

def f_divide(*args):
    n, d = rdiv(args, "/")
    return [VAR_NUM, n // d]

def f_div(*args):
    n, d = rdiv(args, "div")
    return [VAR_NONLIST,
            [[VAR_NUM, n // d],
            [VAR_NUM, n % d]]]

def f_max(*args):
    fn = "max"
    fargt_must_eq(fn, args, 0, VAR_NUM)
    r = args[0][1]
    for i, x in enumerate(args[1:], 1):
        fargt_must_eq(fn, args, i, VAR_NUM)
        if x[1] > r:
            r = x[1]
    return [VAR_NUM, r]

def f_min(*args):
    fn = "min"
    fargt_must_eq(fn, args, 0, VAR_NUM)
    r = args[0][1]
    for i, x in enumerate(args[1:], 1):
        fargt_must_eq(fn, args, i, VAR_NUM)
        if x[1] < r:
            r = x[1]
    return [VAR_NUM, r]

def f_abs(*args):
    fargt_must_eq("abs", args, 0, VAR_NUM)
    return [VAR_NUM, abs(args[0][1])]

def n1_pred(args, fn, pred):
    fargt_must_eq(fn, args, 0, VAR_NUM)
    return [VAR_BOOL, pred(args[0][1])]

def f_zerop(*args):
    return n1_pred(args, "zero?", lambda x: x == 0)

def f_positivep(*args):
    return n1_pred(args, "positive?", lambda x: x > 0)

def f_negativep(*args):
    return n1_pred(args, "negative?", lambda x: x < 0)

def f_evenp(*args):
    return n1_pred(args, "even?", lambda x: x % 2 == 0)

def f_oddp(*args):
    return n1_pred(args, "odd?", lambda x: x % 2 == 1)

def n2_pred(args, fn, pred):
    r = True
    if len(args) == 1:
        fargt_must_eq(fn, args, 0, VAR_NUM)
    elif len(args) > 1:
        fargt_must_eq(fn, args, 0, VAR_NUM)
        n = args[0][1]
        for i, x in enumerate(args[1:], 1):
            fargt_must_eq(fn, args, i, VAR_NUM)
            if not pred(n, x[1]):
                r = False
                break
            n = x[1]
    return [VAR_BOOL, r]

def f_eq(*args):
    return n2_pred(args, "=", lambda x, y: x == y)

def f_lt(*args):
    return n2_pred(args, "<", lambda x, y: x < y)

def f_gt(*args):
    return n2_pred(args, ">", lambda x, y: x > y)

def f_lte(*args):
    return n2_pred(args, "<=", lambda x, y: x <= y)

def f_gte(*args):
    return n2_pred(args, ">=", lambda x, y: x >= y)

# mutation of a variable itself

def setjj(a, b, fn):
    if id(a) == id(b):
        debug("%s self-set %s" % (fn, fargt_repr(a[0])))
    else:
        if var_in(b[0], VAR_LIST | VAR_NONLIST):
            k = to_cons(b)
            b[:] = [VAR_CONS, k]
        a.clear()
        a.extend(b)
    return [VAR_VOID]

def f_setj(*args):
    fn = "set!"
    fargc_must_eq(fn, args, 2)
    if not ((var_in(args[0][0], VAR_LIST | VAR_NONLIST | VAR_CONS)
             and var_in(args[1][0], VAR_LIST | VAR_NONLIST | VAR_CONS))
            or args[0][0] == args[1][0]):
        warning("set! %s with %s"
                % (var_type_name(args[0][0]),
                   var_type_name(args[1][0])))
    return setjj(args[0], args[1], fn)

def f_setjj(*args):
    fn = "set!!"
    fargc_must_eq(fn, args, 2)
    return setjj(args[0], args[1], fn)

def f_dup(*args):
    fargc_must_eq("dup", args, 1)
    if args[0][0] == VAR_VOID:
        warning("dup of void")
        return args[0]
    rc = sys.getrefcount(args[0])
    if rc < 3:
        broken("assumption of refs to an arg")
    if rc == 3:
        return args[0]
    r = [VAR_VOID]
    setjj(r, args[0], "dup")
    return r

# identity

def f_aliasp(*args):
    fargc_must_eq("alias?", args, 2)
    return [VAR_BOOL, id(args[0]) == id(args[1])]

def f_eqp(*args):
    fargc_must_eq("eq?", args, 2)
    if args[0][0] != args[1][0]:
        return [VAR_BOOL, False]
    if var_in(args[0][0], VAR_LIST | VAR_NONLIST):
        assert len(args[0][1]) != 0  # invariant:
        assert len(args[1][1]) != 0  # all empty-lists as consptr null
        return [VAR_BOOL, id(args[0][1]) == id(args[1][1])]
    if (var_in(args[0][0], VAR_CONS | VAR_FUN_OPS | VAR_FUN_HOST)
            or args[0][0] == VAR_DICT):
        return [VAR_BOOL, id(args[0][1]) == id(args[1][1])]
    return [VAR_BOOL, args[0][1] == args[1][1]]

def f_equalp(*args):
    fargc_must_eq("equal?", args, 2)
    # note: DICT do not undergo value-comparison --
    #       it shall differ with itself under equal
    if (not var_in(args[0][0], VAR_LIST | VAR_NONLIST | VAR_CONS)
            or not var_in(args[1][0], VAR_LIST | VAR_NONLIST | VAR_CONS)):
        return f_eqp(*args)
    r = [VAR_BOOL, False]
    if ((args[0][0] == VAR_LIST and args[1][0] == VAR_LIST)
            or (args[0][0] == VAR_NONLIST and args[1][0] == VAR_NONLIST)):
        if len(args[0][1]) != len(args[1][1]):
            return r
        for i, v in enumerate(args[0][1]):
            r = f_equalp(v, args[1][1][i])
            if not r[1]:
                break
        return r
    if args[0][0] == VAR_CONS and args[1][0] == VAR_CONS:
        x = args[0][1]
        y = args[1][1]
        while x is not None:
            if not is_cons(x):
                if is_cons(y):
                    return r
                else:
                    return f_equalp(x, y)
            if y is None or not is_cons(y):
                return r
            if not f_equalp(x.a, y.a)[1]:
                return r
            x = x.d
            y = y.d
        return [VAR_BOOL, y is None]
    if args[0][0] == VAR_CONS or args[1][0] == VAR_CONS:
        if args[1][0] == VAR_CONS:
            is_nonlist = args[0][0] == VAR_NONLIST
            v = args[0][1]
            c = args[1][1]
        else:
            is_nonlist = args[1][0] == VAR_NONLIST
            c = args[0][1]
            v = args[1][1]
        n = len(v)
        i = 0
        while c is not None:
            if i == n:
                return r
            if not is_cons(c):
                if not is_nonlist or i + 1 != n:
                    return r
                return f_equalp(c, v[i])
            if not f_equalp(c.a, v[i])[1]:
                return r
            c = c.d
            i += 1
        return [VAR_BOOL, i == n]
    # note: must now be NONLIST and LIST -- hence not equal
    return r

# support for DICT

# the DICT type supports keys only of the specific types
# NUM or STRING.  the data structure is distinct from the
# impl's own dictionaries:  global environment, the lambda-
# capture environments, the macro-set and the Overlay on the
# global-env and the macro-set (for import).

class Dict:

    def __init__(self, w):
        self.t = w[0][0][0]
        a = []
        for x, y in w:
            if x[0] != self.t:
                raise RunError("dict key types differ")
            a.append((x[1], y))
        self.d = dict(a)

    def ditems(self):
        return [([self.t, k], v) for k, v in self.d.items()]

def f_alist_z_dict(*args):
    fn = "alist->dict"
    fargc_must_eq(fn, args, 1)
    fargt_must_in(fn, args, 0, VAR_LIST | VAR_CONS)
    d = []
    for x in ConsOrListIter(args[0][1]):
        if not var_in(x[0], VAR_CONS | VAR_NONLIST | VAR_LIST):
            raise RunError("not alist")
        d.append((f_car(x), f_cdr(x)))
    if not d:
        return [VAR_DICT, None]
    return [VAR_DICT, Dict(d)]

def f_dict_z_alist(*args):
    fn = "dict->alist"
    fargc_must_eq(fn, args, 1)
    fargt_must_eq(fn, args, 0, VAR_DICT)
    d = [] if args[0][1] is None else args[0][1].ditems()
    return [VAR_LIST, [[VAR_NONLIST, [a, b]] for a, b in d]]

def f_dict_setj(*args):
    fn = "dict-set!"
    fargc_must_eq(fn, args, 3)
    fargt_must_eq(fn, args, 0, VAR_DICT)
    if args[0][1] is None:
        args[0][1] = Dict([[args[1], args[2]]])
        return [VAR_VOID]
    if args[0][1].t != args[2][0]:
        raise RunError("%s: value type %s not dict type %s"
                % (fn, fargt_repr(args[2][0].t), fargt_repr(args[0][1].t)))
    if args[0][1].t != args[1][0]:
        raise RunError("%s: key type %s not dict type %s"
                % (fn, fargt_repr(args[1][0].t), fargt_repr(args[0][1].t)))
    v = args[0][1].d[args[1][1]] = args[2]
    return [VAR_VOID]

def f_dict_get_defaultj(*args):
    fn = "dict-get-default!"
    fargc_must_eq(fn, args, 3)
    fargt_must_eq(fn, args, 0, VAR_DICT)
    if args[0][1] is None:
        args[0][1] = Dict([[args[1], args[2]]])
        return args[2]
    if args[0][1].t != args[2][0]:
        raise RunError("%s: default type %s not dict type %s"
                % (fn, fargt_repr(args[2][0].t), fargt_repr(args[0][1].t)))
    if args[0][1].t != args[1][0]:
        raise RunError("%s: key type %s not dict type %s"
                % (fn, fargt_repr(args[1][0].t), fargt_repr(args[0][1].t)))
    try:
        v = args[0][1].d[args[1][1]]
    except KeyError:
        v = args[0][1].d[args[1][1]] = args[2]
    return v

def f_dict_if_get(*args):
    fn = "dict-if-get"
    fargc_must_eq(fn, args, 4)
    fargt_must_eq(fn, args, 0, VAR_DICT)
    fargt_must_in(fn, args, 3, VAR_FUN_OPS | VAR_FUN_HOST)
    if args[0][1] is None or args[0][1].t != args[1][0]:
        v = args[2]
    else:
        try:
            v = args[0][1].d[args[1][1]]
            return xapply([args[3], v])
        except KeyError:
            v = args[2]
    return v

def f_dictp(*args):
    return typep(args, "dict?", VAR_DICT)

# REC functionality

class Record:

    def __init__(self, nam, values):
        self.nam = nam
        self.values = values

def f_make_record(*args):
    return [VAR_REC, Record(args[0][1], list(args[1:]))]

def f_record_get(*args):
    fn = "record-get"
    fargc_must_eq(fn, args, 2)
    fargt_must_eq(fn, args, 0, VAR_REC)
    fargt_must_eq(fn, args, 1, VAR_NUM)
    return args[0][1].values[args[1][1]]

def f_record_setj(*args):
    fn = "record-set!"
    fargc_must_eq(fn, args, 3)
    fargt_must_eq(fn, args, 0, VAR_REC)
    fargt_must_eq(fn, args, 1, VAR_NUM)
    args[0][1].values[args[1][1]] = args[2]
    return [VAR_VOID]

def f_recordp(*args):
    fn = "record?"
    fargc_must_eq(fn, args, 2)
    fargt_must_eq(fn, args, 0, VAR_REC)
    fargt_must_eq(fn, args, 1, VAR_NAM)
    return [VAR_BOOL, args[0][1].nam == args[1][1]]

# STRING functions

def f_string_ref(*args):
    fn = "string-ref"
    fargc_must_eq(fn, args, 2)
    fargt_must_eq(fn, args, 1, VAR_NUM)
    fargt_must_eq(fn, args, 0, VAR_STRING)
    return [VAR_NUM, args[0][1][args[1][1]]]

def f_string_z_list(*args):
    fargt_must_eq("string->list", args, 0, VAR_STRING)
    return [VAR_LIST, [[VAR_NUM, ord(c)] for c in args[0][1]]]

def f_list_z_string(*args):
    fargt_must_in("list->string", args, 0, VAR_LIST | VAR_CONS)
    return [VAR_STRING, "".join([chr(n[1])
        for n in ConsOrListIter(args[0][1])])]

def f_symbol_z_string(names):
    def to_string(*args):
        fargt_must_eq("symbol->string", args, 0, VAR_NAM)
        return [VAR_STRING, names[args[0][1]]]
    return to_string

def f_substring(*args):
    fn = "substring"
    fargc_must_ge(fn, args, 2)
    fargt_must_eq(fn, args, 0, VAR_STRING)
    fargt_must_eq(fn, args, 1, VAR_NUM)
    s = args[0][1]
    i = args[1][1]
    if len(args) >= 3:
        fargt_must_eq(fn, args, 2, VAR_NUM)
        j = args[2][1]
        r = s[i:j]
    else:
        r = s[i:]
    return [VAR_STRING, r]

def f_substring_index(*args):
    fn = "substring-index"
    fargc_must_ge(fn, args, 2)
    fargt_must_eq(fn, args, 0, VAR_STRING)
    fargt_must_eq(fn, args, 1, VAR_STRING)
    u = args[0][1]
    s = args[1][1]
    i = 0
    if len(args) >= 3:
        fargt_must_eq(fn, args, 2, VAR_NUM)
        i = args[2][1]
    return [VAR_NUM, s.find(u, i)]

def f_string_length(*args):
    fn = "string-length"
    fargc_must_ge(fn, args, 1)
    fargt_must_eq(fn, args, 0, VAR_STRING)
    return [VAR_NUM, len(args[0][1])]

def f_string_append(*args):
    r = []
    for (i, x) in enumerate(args):
        fargt_must_eq("string-append", args, i, VAR_STRING)
        r.append(x[1])
    return [VAR_STRING, "".join(r)]

def stringpred(args, fn, pred):
    fargc_must_eq(fn, args, 2)
    fargt_must_eq(fn, args, 0, VAR_STRING)
    fargt_must_eq(fn, args, 1, VAR_STRING)
    return [VAR_BOOL, pred(args[0][1], args[1][1])]

def f_stringeqp(*args):
    return stringpred(args, "string=?", lambda x, y: x == y)

def f_stringltp(*args):
    return stringpred(args, "string<?", lambda x, y: x < y)

def f_stringgtp(*args):
    return stringpred(args, "string>?", lambda x, y: x > y)

def f_string_z_number(*args):
    fn = "string->number"
    fargc_must_ge(fn, args, 1)
    fargt_must_eq(fn, args, 0, VAR_STRING)
    radix = 0
    if len(args) > 1:
        fargt_must_eq(fn, args, 1, VAR_NUM)
        radix = args[1][1]
    try:
        return [VAR_NUM, int(args[0][1], radix)]
    except ValueError:
        raise RunError("%s %s with radix %d"
                % (fn, args[0][1], radix))

def f_number_z_string(*args):
    fn = "number->string"
    fargc_must_ge(fn, args, 1)
    fargt_must_eq(fn, args, 0, VAR_NUM)
    b = "d"
    if len(args) > 1:
        fargt_must_eq(fn, args, 1, VAR_NUM)
        radix = args[1][1]
        if radix == 2:
            b = "b"
        elif radix == 8:
            b = "o"
        elif radix == 10:
            pass
        elif radix == 16:
            b = "x"
        else:
            raise RunError("%s %s with radix %d"
                    % (fn, args[0][1], radix))
    return [VAR_STRING, format(args[0][1], b)]

# type checks

def typep(args, fn, t):
    fargc_must_eq(fn, args, 1)
    return [VAR_BOOL, t == args[0][0]]

def f_booleanp(*args):
    return typep(args, "boolean?", VAR_BOOL)

def f_numberp(*args):
    return typep(args, "number?", VAR_NUM)

def f_procedurep(*args):
    fn = "procedure?"
    return [VAR_BOOL, var_in(args[0][0],
        VAR_FUN_OPS | VAR_FUN_HOST)]

def f_symbolp(*args):
    return typep(args, "symbol?", VAR_NAM)

def f_nullp(*args):
    r = typep(args, "null?", VAR_LIST)
    if r[1]:
        r[1] = (0 == len(args[0][1]))
        if r[1]: broken("null as const-list")
    elif args[0][0] == VAR_CONS:
        r[1] = (None == args[0][1])
    return r

def f_listp(*args):
    def get_last(c):
        warning("cons-iter for count")
        while c.d is not None:
            if not is_cons(c.d):
                return c
            c = c.d
        return c
    r = typep(args, "list?", VAR_LIST)
    if r[1]:
        r[1] = (0 != len(args[0][1]))
        if not r[1]: broken("null as cont-list")
    elif args[0][0] == VAR_CONS:
        r[1] = not (args[0][1] is None
                or get_last(args[0][1]).d is not None)
    return r

def f_pairp(*args):
    fargc_must_eq("pair?", args, 1)
    if args[0][0] == VAR_NONLIST:
        return [VAR_BOOL, True]
    if args[0][0] == VAR_LIST:
        r = [VAR_BOOL, len(args[0][1]) != 0]
        if not r[1]: broken("null as cont-list")
        return r
    if args[0][0] == VAR_CONS:
        return [VAR_BOOL, args[0][1] is not None]
    return [VAR_BOOL, False]

def f_contpp(*args):
    fargc_must_eq("cont??", args, 1)
    return [VAR_BOOL, var_in(args[0][0], VAR_LIST | VAR_NONLIST)]

def f_voidp(*args):
    fargc_must_eq("void?", args, 1)
    return [VAR_BOOL, args[0][0] == VAR_VOID]

# list functions

def f_length(*args):
    fn = "length"
    fargc_must_eq(fn, args, 1)
    fargt_must_in(fn, args, 0, VAR_CONS | VAR_LIST)
    if args[0][0] == VAR_CONS:
        if args[0][1] is None:
            r = 0
        else:
            r = args[0][1].length()
    else:
        r = len(args[0][1])
    return [VAR_NUM, r]

def f_apply(*args):
    fn = "apply"
    fargc_must_eq(fn, args, 2)
    fargt_must_in(fn, args, 1, VAR_CONS | VAR_LIST)
    return xapply([args[0]] + normal_list(args[1][1]))

def f_map(*args):
    fn = "map"
    fargt_must_in(fn, args, 0, VAR_FUN_OPS | VAR_FUN_HOST)
    f = args[0]
    inputs = []
    for i in range(1, len(args)):
        fargt_must_in(fn, args, i, VAR_LIST | VAR_CONS)
        inputs.append(ConsOrListIter(args[i][1]))
    r = []
    try:
        while True:
            w = []
            for j in range(len(inputs)):
                x = next(inputs[j])
                w.append(x)
            r.append(fun_call([f] + w))
    except StopIteration:
        pass
    return [VAR_LIST, r]

def search_pred(a):
    if var_in(a[0], VAR_FUN_OPS | VAR_FUN_HOST):
        p = lambda x: fun_call([a, x])
    else:
        p = lambda x: f_equalp(a, x)
    def t(y):
        v = p(y)
        return v[0] != VAR_BOOL or v[1]
    return t

def f_member(*args):
    f = [VAR_BOOL, False]
    fargt_must_in("member", args, 1, VAR_CONS | VAR_LIST | VAR_NONLIST)
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
            return f_dup(args[1])
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

def f_assoc(*args):
    f = [VAR_BOOL, False]
    fargt_must_in("assoc", args, 1, VAR_CONS | VAR_LIST | VAR_NONLIST)
    t = search_pred(args[0])
    if args[1][0] == VAR_CONS:
        r = args[1][1]
        while r:
            if not is_cons(r):
                warning("assoc hit non-cons cdr")
                break
            x = r.a
            if t(f_car(x)):
                return x
            r = r.d
    else:
        for x in args[1][1]:
            if t(f_car(x)):
                return x
    return f

# display functions (see also, I/O functions)
#
#   for development purposes and as of r7rs, not suggested for
#   program utilization

def f_display(names):
    def display(*args):
        r = []
        for a in args:
            if a[0] == VAR_STRING:
                r.append(a[1])
            else:
                r.append(vrepr(a, names))
        sys.stdout.write(" ".join(r))
        sys.stdout.flush()
        return [VAR_VOID]
    return display

# serialization

def f_read(names, env):
    def rd(*args):
        fn = "read"
        fargc_must_eq(fn, args, 1)
        fargt_must_eq(fn, args, 0, VAR_STRING)
        t = readx(args[0][1], names)
        if len(t) == 0:
            return [VAR_VOID]
        if len(t) != 1:
            warning("trailing objects")
        return from_lex(t[0])
    return rd

def f_write(names):
    def rd(*args):
        fn = "write"
        fargc_must_eq(fn, args, 1)
        s = vrepr(args[0], names)
        return [VAR_STRING, s]
    return rd

# I/O functions

def f_error(names):
    def err(*args):
        sys.stderr.write("error-function invoked,\n")
        for i, a in enumerate(args):
            sys.stdout.write("error-args[%d]: %s\n" % (i, vrepr(a, names)))
        sys.stderr.write("-- sorry'bout that\n")
        sys.exit(1)
    return err

def f_exit(*args):
    fargt_must_eq("exit", args, 0, VAR_NUM)
    sys.exit(args[0][1])

def read_line_(read_byte):
    a = []
    while True:
        b = read_byte()
        if not b:
            if not a:
                return None
            break
        c = ord(b)
        if c == 10:
            break
        a.append(c)
    return bytes(a).decode("utf-8")

class File:

    def __init__(self, f):
        self.f = f

    def __del__(self):
        self.f.close()

    def read_byte(self):
        return self.f.read(1)

    def read_line(self):
        return read_line_(self.read_byte)

    def _complete_write(self, b):
        i = 0
        n = len(b)
        while i != n:
            i += self.f.write(b[i:])

    def write_byte(self, i):
        self._complete_write(bytes([i]))

    def write_string(self, s):
        self._complete_write(s.encode("utf-8"))

class SystemFile(File):

    def __init__(self, f):
        super().__init__(f.buffer)

    def __del__(self):
        pass

def f_open_input_file(*args):
    fn = "open-input-file"
    fargc_must_eq(fn, args, 1)
    fargt_must_eq(fn, args, 0, VAR_STRING)
    try:
        return [VAR_PORT, File(open(args[0][1], "rb"))]
    except:
        return [VAR_BOOL, False]

def f_open_output_file(*args):
    fn = "open-output-file"
    fargc_must_eq(fn, args, 1)
    fargt_must_eq(fn, args, 0, VAR_STRING)
    try:
        return [VAR_PORT, File(open(args[0][1], "wb"))]
    except:
        return [VAR_BOOL, False]

def f_eof_objectp(*args):
    return typep(args, "eof-object?", VAR_EOF)

def f_read_byte(*args):
    fn = "read-byte"
    fargc_must_eq(fn, args, 1)
    fargt_must_eq(fn, args, 0, VAR_PORT)
    f = args[0][1]
    b = f.read_byte()
    if b is None:
        return [VAR_EOF]
    return [VAR_NUM, ord(b)]

def f_read_line(*args):
    fn = "read-line"
    fargc_must_eq(fn, args, 1)
    fargt_must_eq(fn, args, 0, VAR_PORT)
    f = args[0][1]
    s = f.read_line()
    if s is None:
        return [VAR_EOF]
    return [VAR_STRING, s]

def f_write_byte(*args):
    fn = "write-byte"
    fargc_must_eq(fn, args, 2)
    fargt_must_eq(fn, args, 0, VAR_NUM)
    fargt_must_eq(fn, args, 1, VAR_PORT)
    args[1][1].write_byte(args[0][1])
    return [VAR_VOID]

def f_write_string(*args):
    fn = "write-string"
    fargc_must_eq(fn, args, 2)
    fargt_must_eq(fn, args, 0, VAR_STRING)
    fargt_must_eq(fn, args, 1, VAR_PORT)
    args[1][1].write_string(args[0][1])
    return [VAR_VOID]

import subprocess

def f_with_input_pipe(*args):
    fn = "with-input-pipe"
    fargc_must_eq(fn, args, 2)
    fargt_must_in(fn, args, 0, VAR_LIST | VAR_CONS)
    fargt_must_in(fn, args, 1, VAR_FUN_OPS | VAR_FUN_HOST)
    a = []
    for e in ConsOrListIter(args[0][1]):
        fchk_or_fail(e[0] == VAR_STRING, "%s got %s expects string"
                % (fn, fargt_repr(e[0])))
        a.append(e[1])
    u = subprocess.Popen(a, stdout=subprocess.PIPE)
    f = File(u.stdout)
    p = [VAR_PORT, f]
    r = fun_call([args[1], p])
    del f, p
    c = u.wait()
    return [VAR_NONLIST, [[VAR_NUM, c], r]]

def f_with_output_pipe(*args):
    fn = "with-output-pipe"
    fargc_must_eq(fn, args, 2)
    fargt_must_in(fn, args, 0, VAR_LIST | VAR_CONS)
    fargt_must_in(fn, args, 1, VAR_FUN_OPS | VAR_FUN_HOST)
    a = []
    for e in ConsOrListIter(args[0][1]):
        fchk_or_fail(e[0] == VAR_STRING, "%s got %s expects string"
                % (fn, fargt_repr(e[0])))
        a.append(e[1])
    u = subprocess.Popen(a, stdin=subprocess.PIPE)
    f = File(u.stdin)
    p = [VAR_PORT, f]
    r = fun_call([args[1], p])
    del f, p  # contract:  port reference kept in fun will hang here
    c = u.wait()
    return [VAR_NONLIST, [[VAR_NUM, c], r]]

class InStringFile:

    def __init__(self, b):
        self.i = 0
        self.b = b

    def read_byte(self):
        if self.i == len(self.b):
            return None
        r = self.b[self.i]
        self.i += 1
        return chr(r)

    def read_line(self):
        return read_line_(self.read_byte)

def f_open_input_string(*args):
    fn = "open-input-string"
    fargc_must_eq(fn, args, 1)
    fargt_must_eq(fn, args, 0, VAR_STRING)
    b = args[0][1].encode("utf-8")
    return [VAR_PORT, InStringFile(b)]

def f_open_input_string_bytes(*args):
    fn = "open-input-string-bytes"
    fargc_must_eq(fn, args, 1)
    fargt_must_in(fn, args, 0, VAR_LIST | VAR_CONS)
    b = []
    for e in ConsOrListIter(args[0][1]):
        fchk_or_fail(e[0] == VAR_NUM, "%s got %s expects number"
                % (fn, fargt_repr(e[0])))
        b.append(e[1])
    return [VAR_PORT, InStringFile(b)]

class OutStringFile:

    def __init__(self):
        self.b = []

    def write_byte(self, i):
        self.b.append(i)

    def write_string(self, s):
        self.b.extend(s.encode("utf-8"))

def f_open_output_string(*args):
    fargc_must_eq("open-output-string", args, 0)
    return [VAR_PORT, OutStringFile()]

def f_output_string_get(*args):
    fn = "output-string-get"
    fargc_must_eq(fn, args, 1)
    fargt_must_eq(fn, args, 0, VAR_PORT)
    fchk_or_fail(isinstance(args[0][1], OutStringFile),
            "%s got output-port")
    return [VAR_STRING, bytes(args[0][1].b).decode("utf-8")]

def f_output_string_get_bytes(*args):
    fn = "output-string-get-bytes"
    fargc_must_eq(fn, args, 1)
    fargt_must_eq(fn, args, 0, VAR_PORT)
    fchk_or_fail(isinstance(args[0][1], OutStringFile),
            "%s got output-port")
    r = []
    for i in args[0][1].b:
        r.append([VAR_NUM, i])
    return [VAR_LIST, r]

command_line = None

def f_system_command_line(*args):
    fargc_must_eq("system-command-line", args, 0)
    r = []
    for s in command_line:
        r.append([VAR_STRING, s])
    return [VAR_LIST, r]

def system_f(n, f, fn):
    if n: fargs_count_fail(fn, 0, n)
    return [VAR_PORT, SystemFile(f)]

def f_system_input_file(*args):
    return system_f(len(args), sys.stdin, "system-input-file")

def f_system_output_file(*args):
    return system_f(len(args), sys.stdout, "system-output-file")

def f_system_error_file(*args):
    return system_f(len(args), sys.stderr, "system-error-file")

# prng function

from random import Random

def f_make_prng(*args):
    fn = "make-prng"
    fargc_must_eq(fn, args, 1)
    fargt_must_eq(fn, args, 0, VAR_NUM)
    seed = args[0][1]
    prng = Random(seed)
    def r(*args):
        fn = "prng"
        fargc_must_ge(fn, args, 1)
        fargt_must_eq(fn, args, 0, VAR_NUM)
        b = args[0][1]
        if len(args) > 1:
            fargt_must_eq(fn, args, 1, VAR_NUM)
            a, b = b, args[1][1]
        return [VAR_NUM, prng.randint(a, b)]
    return [VAR_FUN_HOST, r]

# system time functions

from time import time, sleep

def f_clock(*args):
    fargc_must_eq("clock", args, 0)
    return [VAR_NUM, int(time())]

JIFFIES_PER_SECOND = 1000
zt = None

def f_current_jiffy(*args):
    fargc_must_eq("current-jiffy", args, 0)
    r = 0
    global zt
    if not zt:
        zt = time()
    else:
        r = int((time() - zt) * JIFFIES_PER_SECOND)
    return [VAR_NUM, r]

def f_pause(*args):
    fn = "pause"
    fargc_must_eq(fn, args, 1)
    fargt_must_eq(fn, args, 0, VAR_NUM)
    p = args[0][1] / JIFFIES_PER_SECOND
    if p > 0:
        sleep(p)
    return [VAR_VOID]

##
# the eval loop
##

class FunOps:

    def is_last_(w, block):
        return id(w) == id(block[-1])

    def __init__(self, dot, captured, le, block):
        self.dot = dot
        self.captured = captured
        self.le = le
        self.block = block

    def guts(self):
        return self.dot, self.captured, self.le, self.block

    def tco(self, args):
        dot, captured, le, block = self.guts()
        v = None
        done = False
        while not done:
            env = le.activation(captured, dot, args)
            done = True
            for w in block:
                v = xeval(w, env)
                if v[0] == VAR_APPLY:
                    a = v[1]
                    f = a[0]
                    assert f[0] == VAR_FUN_OPS
                    args = a[1:]
                    if not FunOps.is_last_(w, block):
                        debug("rec-apply", a)
                        v = f[1].tco(args)
                    else:
                        debug("iter-apply", a)
                        dot, captured, le, block = f[1].guts()
                        done = False
        return v


def fun_call(a):
    debug("fun-call", a)
    f = a[0]
    args = a[1:]
    # note: native function call, for builtins
    if f[0] == VAR_FUN_HOST:
        debug("native-fun")
        a = f[1](*args)
        if a[0] != VAR_APPLY:
            return a
        b = a[1]
        f = b[0]
        args = b[1:]
    assert f[0] == VAR_FUN_OPS
    return f[1].tco(args)

def make_fun(up, x, lambda_op):
    local_env = x[0]
    captured = []
    for k in x[1]:
        captured.append(up[k])
    fun_block = x[2:]
    dot = (lambda_op == OP_LAMBDA_DOT)
    return [VAR_FUN_OPS, FunOps(dot, captured, local_env, fun_block)]

def flatten_splices(a):
    r = []
    for v in a:
        if v[0] == VAR_SPLICE:
            r.extend(v[1])
        else:
            r.append(v)
    return r

def run_each(x, env):
    return flatten_splices([run(y, env) for y in x])

i_env = {}

def xeval(x, env):
    debug("eval", x)
    if type(x) != list:
        if x[0] == LEX_LIST:
            r = run_each(x[1], env)
            if len(r) == 0:
                return [VAR_CONS, None]
            return [VAR_LIST, r]
        if x[0] == LEX_NONLIST:
            r = run_each(x[1], env)
            if r[-1][0] == VAR_LIST:
                return [VAR_LIST, r[:-1] + r[-1][1]]
            if r[-1][0] == VAR_CONS:
                c = Cons.from_list(r[:-1])
                Cons.last.d = r[-1][1]
                return [VAR_CONS, c]
            return [VAR_NONLIST, r]
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
        if x[0] == LEX_REC:
            n = x[1][0]
            if n[0] != LEX_SYM:
                broken("record-id not symbol")
            return [VAR_REC, Record(n[1], run_each(x[1][1:], env))]
        if x[0] in (LEX_QUOTE, LEX_QUASIQUOTE):
            broken("eval quote")
        if x[0] == LEX_UNQUOTE:
            return [VAR_UNQUOTE, x[1]]
        if x[0] == LEX_VOID:
            return [VAR_VOID]
        if x[0] == LEX_DOT:
            raise RunError("invalid use of dot")
        # in some cases I could have catched at parse time
        # which means it would have been a lex error.
        broken(x)
    if len(x) == 0:
        broken("empty form")
    if type(x[0]) == int:
        return xeval_op(x, env)
    return xapply(run_each(x, env))

def xeval_op(x, env):
    if x[0] == OP_BIND:
        v = run(x[2], env)
        env[x[1]] = v
        return [VAR_VOID]
    if x[0] in (OP_LAMBDA, OP_LAMBDA_DOT):
        return make_fun(env, x[1:], x[0])
    if x[0] == OP_COND:
        for y in x[1:]:
            t = run(y[0], env)
            if t[0] != VAR_BOOL or t[1]:
                return xeval(y[1], env)
        raise RunError("all cond #f")
    if x[0] == OP_IMPORT:
        e = Overlay(i_env)
        for z in x[2:]:
            run(z, e)
        for (a, b) in x[1].items():
            if b not in e:
                # note: could instead have been checked on parse
                #       of the import by performing unbound there
                raise RunError("no %s for export" % (names[b],))
            env[a] = e[b]
        return [VAR_VOID]
    if x[0] == OP_SEQ:
        for y in x[1:]:
            r = run(y, env)
        return r
    if x[0] == OP_EXPORT:
        return [VAR_VOID]
    broken("unknown form")

def xapply(a):
    if not var_in(a[0][0], VAR_FUN_OPS | VAR_FUN_HOST):
        raise RunError("apply %s"
                % (fargt_repr(a[0][0])))
    if a[0][0] == VAR_FUN_OPS:
        return [VAR_APPLY, a]
    return a[0][1](*a[1:])

def run(x, env):
    y = xeval(x, env)
    if y[0] != VAR_APPLY:
        return y
    a = y[1]
    f = a[0]
    return f[1].tco(a[1:])

##
# init
##

def with_new_name(name, f, d, names):
    i = len(names)
    if i != names.intern(name):
        broken("not new")
    d[i] = f

def init_macros(env_keys, names, opener):
    macros = dict()
    macros[NAM_QUOTE] = m_quote
    macros[NAM_QUASIQUOTE] = m_quasiquote
    macros[NAM_UNQUOTE] = m_unquote
    macros[NAM_MACRO] = m_macro(names, macros)
    macros[NAM_IMPORT] = m_import(names, macros, opener)
    with_new_name("gensym", m_gensym(names), macros, names)
    for a, b in [
            ("ref", m_ref),
            ("define", m_define),
            ("seq", m_seq),
            ("lambda", m_lambda),
            ("let", m_let),
            ("let*", m_letx),
            ("letrec", m_letrec),
            ("letrec*", m_letrecx),
            ("cond", m_cond),
            ("case", m_case),
            ("do", m_do),
            ("begin", m_begin),
            ("if", m_if),
            ("and", m_and),
            ("or", m_or),
            ("when", m_when),
            ("unless", m_unless),
            ("export", m_export)]:
        with_new_name(a, b, macros, names)

    with_new_name("scope", m_scope(names, env_keys), macros, names)
    return macros

def init_env(names):
    env = dict()
    env[NAM_CAR] = [VAR_FUN_HOST, f_car]
    env[NAM_EQVP] = [VAR_FUN_HOST, f_eqp]  # impl: eq? is identical as eqv?
    env[NAM_LIST] = [VAR_FUN_HOST, f_list]
    env[NAM_NONLIST] = [VAR_FUN_HOST, f_nonlist]
    env[NAM_SETJJ] = [VAR_FUN_HOST, f_setjj]
    env[NAM_DUP] = [VAR_FUN_HOST, f_dup]
    env[NAM_ERROR] = [VAR_FUN_HOST, f_error(names)]
    env[NAM_SPLICE] = [VAR_FUN_HOST, f_splice]
    with_new_name("display", [VAR_FUN_HOST, f_display(names)], env, names)
    with_new_name("symbol->string",
                  [VAR_FUN_HOST, f_symbol_z_string(names)], env, names)
    with_new_name("read", [VAR_FUN_HOST, f_read(names, env)], env, names)
    with_new_name("write", [VAR_FUN_HOST, f_write(names)], env, names)
    for a, b in [
            ("+", f_pluss),
            ("-", f_minus),
            ("*", f_multiply),
            ("/", f_divide),
            ("div", f_div),
            ("max", f_max),
            ("min", f_min),
            ("zero?", f_zerop),
            ("negative?", f_negativep),
            ("positive?", f_positivep),
            ("even?", f_evenp),
            ("odd?", f_oddp),
            ("list-copy", f_list_copy),
            ("list-ref", f_list_ref),
            ("list-tail", f_list_tail),
            ("list-set!", f_list_setj),
            ("make-list", f_make_list),
            ("length", f_length),
            ("apply", f_apply),
            ("reverse", f_reverse),
            ("take", f_take),
            ("member", f_member),
            ("assoc", f_assoc),
            ("set!", f_setj),
            ("alias?", f_aliasp),
            ("eq?", f_eqp),
            ("equal?", f_equalp),
            ("cdr", f_cdr),
            ("set-car!", f_set_carj),
            ("set-cdr!", f_set_cdrj),
            ("append", f_append),
            ("cons", f_cons),
            ("boolean?", f_booleanp),
            ("number?", f_numberp),
            ("procedure?", f_procedurep),
            ("symbol?", f_symbolp),
            ("null?", f_nullp),
            ("list?", f_listp),
            ("pair?", f_pairp),
            ("cont??", f_contpp),
            ("void?", f_voidp),
            ("not", f_not),
            ("=", f_eq),
            ("<", f_lt),
            (">", f_gt),
            ("<=", f_lte),
            (">=", f_gte),
            ("map", f_map),
            ("abs", f_abs),
            ("alist->dict", f_alist_z_dict),
            ("dict->alist", f_dict_z_alist),
            ("dict-set!", f_dict_setj),
            ("dict-get-default!", f_dict_get_defaultj),
            ("dict-if-get", f_dict_if_get),
            ("dict?", f_dictp),
            ("make-record", f_make_record),
            ("record-get", f_record_get),
            ("record-set!", f_record_setj),
            ("record?", f_recordp),
            ("string->list", f_string_z_list),
            ("list->string", f_list_z_string),
            ("string-ref", f_string_ref),
            ("substring", f_substring),
            ("substring-index", f_substring_index),
            ("string-length", f_string_length),
            ("string-append", f_string_append),
            ("string=?", f_stringeqp),
            ("string<?", f_stringltp),
            ("string>?", f_stringgtp),
            ("string->number", f_string_z_number),
            ("number->string", f_number_z_string),
            ("exit", f_exit),
            ("eof-object?", f_eof_objectp),
            ("open-input-file", f_open_input_file),
            ("open-output-file", f_open_output_file),
            ("read-byte", f_read_byte),
            ("read-line", f_read_line),
            ("write-byte", f_write_byte),
            ("write-string", f_write_string),
            ("with-input-pipe", f_with_input_pipe),
            ("with-output-pipe", f_with_output_pipe),
            ("open-output-string", f_open_output_string),
            ("output-string-get", f_output_string_get),
            ("output-string-get-bytes", f_output_string_get_bytes),
            ("open-input-string", f_open_input_string),
            ("open-input-string-bytes", f_open_input_string_bytes),
            ("system-command-line", f_system_command_line),
            ("system-input-file", f_system_input_file),
            ("system-output-file", f_system_output_file),
            ("system-error-file", f_system_error_file),
            ("make-prng", f_make_prng),
            ("clock", f_clock),
            ("current-jiffy", f_current_jiffy),
            ("pause", f_pause)]:
        with_new_name(a, [VAR_FUN_HOST, b], env, names)

    return env

def run_top(ast, env, names):
    try:
        for a in ast:
            r = run(a, env)
            if r[0] != VAR_VOID:
                result(vrepr(r, names))
    except KeyError as e:
        i = e.args[0]
        if i <= len(names):
            raise RunError("no entry %s (%d)" % (names[i], i))
        raise CoreError("internal key error: %d" % (i,))

import itertools

def inc_functions(names, env, macros):
    def cxr(s):
        if not s:
            return "x"
        n, r = s[0], cxr(s[1:])
        if n.isdigit():
            return "(list-ref %s %s)" % (r, n)
        return "(c%sr %s)" % (n, r)
    a = []
    for w in itertools.product( * ["ad"] * 2):
        s = "".join(w).replace("ad", "1")
        a.append("(ref (c%s%sr x) %s)" % tuple(list(w) + [cxr(s)]))
    for w in itertools.product( * ["ad"] * 3):
        s = "".join(w).replace("add", "2").replace("ad", "1")
        a.append("(ref (c%s%s%sr x) %s)" % tuple(list(w) + [cxr(s)]))
    for w in itertools.product( * ["ad"] * 4):
        s = "".join(w).replace("addd", "3").replace("add", "2").replace("ad", "1")
        a.append("(ref (c%s%s%s%sr x) %s)" % tuple(list(w) + [cxr(s)]))
    t = compx("\n".join(a), names, macros, env.keys())
    run_top(t, env, names)

def inc_macros(names, macros):
    s = """
(macro case-lambda args
`(lambda =>
   (apply (case (length =>)
       ,@(map (lambda (=>) `((,(length (car =>)))
                               (lambda ,@=>)))
         args)) =>)))
(macro ref@ args
  (ref n (length args))
  (ref n_1 (- n 1))
  (ref syms (take n_1 args))
  (ref s (list-ref args n_1))
  (ref ls (gensym))
  (ref defs (let loop ((i 0) (r '()))
     (if (equal? i n_1) r
       (loop (+ 1 i)
         (cons `(ref ,(list-ref syms i) (list-ref ,ls ,i)) r))
     )))
  `(seq (ref ,ls ,s) ,@defs)
)
(macro define-record-type (name constructor pred . accessors)
  (ref (getter x i)
    `(ref (,(cadr x) v) (record-get v ,i)))
  (ref getter-defs
    (let loop ((d accessors) (r '()) (i 0))
      (if (null? d) r
        (loop (cdr d) (cons (getter (car d) i) r) (+ i 1))
    )))
  (ref (setter x i)
    (if (eq? (length x) 2) #f
      `(ref (,(caddr x) v y) (record-set! v ,i y))))
  (ref setter-defs
    (let loop ((d accessors) (r '()) (i 0))
      (if (null? d) r
        (let ((e (setter (car d) i)))
          (loop (cdr d) (if e (cons e r) r) (+ i 1))
    ))))
  (ref c-args (cdr constructor))
  `(seq
    (ref ,constructor (make-record ',name @`,(list ,@c-args)))
    (ref (,pred v) (record? v ',name))
    ,@getter-defs
    ,@setter-defs
))
(macro class procs
  (ref (gen-case proc)
    `((',proc) ,proc))
  `(lambda (cmd . args)
     (apply
       (case cmd
         ,@(map gen-case procs)
         (else => error))
       args)))
(macro local args
  (cons 'seq (map (lambda (n) `(define ,n ,n)) args)))
"""
    env = i_env
    t = compx(s, names, macros, env.keys())
    run_top(t, env, names)

def init_top(opener, extra_f=()):
    names = Names(
            (NAM_THEN, "=>"),
            (NAM_ELSE, "else"),
            (NAM_QUOTE, "quote"),
            (NAM_QUASIQUOTE, "quasiquote"),
            (NAM_UNQUOTE, "unquote"),
            (NAM_MACRO, "macro"),
            (NAM_CAR, "car"),
            (NAM_EQVP, "eqv?"),
            (NAM_LIST, "list"),
            (NAM_NONLIST, "nonlist"),
            (NAM_SETJJ, "set!!"),
            (NAM_DUP, "dup"),
            (NAM_ERROR, "error"),
            (NAM_SPLICE, "splice"),
            (NAM_IMPORT, "import"))
    env = init_env(names)
    for a, b in extra_f:
        with_new_name(a, [VAR_FUN_HOST, b], env, names)

    macros = init_macros(env.keys(), names, opener)

    opener.filename = "inc-functions"
    inc_functions(names, env, macros)
    global i_env
    i_env = dict(env)

    opener.filename = "inc-macros"
    inc_macros(names, macros)
    global i_macros
    i_macros = dict(macros)

    opener.filename = None
    return names, env, macros

##
# ui
##

def xrepr(s, names):
    if not s:
        return "#<>"
    if type(s) == list:
        if type(s[0]) != int:
            return "(%s)" % (" ".join(xrepr(x, names) for x in s),)
        if s[0] == OP_BIND:
            return "#<bind %s %s #>" % (
                    names[s[1]], xrepr(s[2], names))
        if s[0] in (OP_LAMBDA, OP_LAMBDA_DOT):
            return "#<lambda #>"
            # Omitted:  Content of lambda, as names only contains
            # the local env (as invoked from vrepr), and I would
            # need the complete interned names lookup to recurse
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
        broken("unhandled %r" % (s,))
    if lex_in(s[0], LEX_LIST | LEX_NONLIST):
        w  = [xrepr(x, names) for x in s[1]]
        if s[0] == LEX_NONLIST:
            w.insert(-1, ".")
        return "$(%s)" % " ".join(w)
    if s[0] == LEX_SYM:
        return "'#|%d|#" % (s[1],)
    if s[0] == LEX_NAM:
        return names[s[1]]
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

def vrepr(s, names, q=None):
    if var_in(s[0], VAR_LIST | VAR_NONLIST):
        w  = [vrepr(x, names, q) for x in s[1]]
        if s[0] == VAR_NONLIST:
            w.insert(-1, ".")
        return "(%s)" % " ".join(w)
    if s[0] == VAR_SPLICE:
        return "#@(%s)" % " ".join(
                vrepr(x, names, q) for x in s[1])
    if s[0] == VAR_CONS:
        if s[1] is None:
            return "()"
        return "%s" % vrepr(s[1].to_list_var(), names, q)
    if s[0] == VAR_NAM:
        n = names[s[1]]
        return n
    if s[0] == VAR_NUM:
        return str(s[1])
    if s[0] == VAR_STRING:
        return '"' + s[1].replace('"', '\\"') + '"'
    if s[0] == VAR_REC:
        return "#r(%s %s)" % (names[s[1].nam], " ".join(
            vrepr(x, names, q) for x in s[1].values))
    if s[0] == VAR_DICT:
        if s[1] is None:
            r = ""
        elif verbose:
            r = " # ".join(
                    ["%s %s" % (vrepr(x, names, q),
                                vrepr(y, names, q))
                     for x, y in s[1].ditems()])
        else:
            r = "#"
        return "#{%s}" % (r,)
    if var_in(s[0], VAR_FUN_OPS | VAR_FUN_HOST):
        if s[0] == VAR_FUN_HOST:
            return "#~fun-host"
        if not verbose:
            return "#~fun"
        le = s[2]
        p = le.n_parms
        lnames = [names[k] for k in le.names]
        g = id(s[1])
        if not q:
            q = set()
        elif g in q:
            return "..."
        q.add(g)
        return "#~fun(%s)[%s]{ %s }" % (
                " ".join(lnames[:p]),
                "|".join("%s %s"
                    % (lnames[k], vrepr(s[1][k - p], names, q))
                    for k in range(p, le.n_init)),
                " ".join(xrepr(x, lnames) for x in s[3]))
    if s[0] == VAR_PORT:
        return "#~port"
    if s[0] == VAR_EOF:
        return "#~eof-object"
    if s[0] == VAR_BOOL:
        if type(s[1]) != bool:
            warning("dirty bool")
        return "#" + "ft"[bool(s[1])]
    if s[0] == VAR_VOID:
        return "#void"
    if s[0] == VAR_QUOTE:
        return "#~quote %s" % xrepr(s[1], names)
    if s[0] == VAR_UNQUOTE:
        return "#~unquote %s" % xrepr(s[1], names)
    if s[0] >= VAR_EXTRA_MIN and s[0] <= VAR_EXTRA_MAX:
        return "#~%s" % (var_type_name(s[0]),)
    return "#~ %r" % (s,)

# "nc" extension

VAR_SCR = VAR_EXTRA_MIN + 0

import curses

def ef_nc_initscr(*args):
    fn = "nc-initscr"
    tenths = None
    if len(args) > 0:
        fargt_must_eq(fn, args, 0, VAR_NUM)
        tenths = args[0][1]
    stdscr = curses.initscr()
    curses.noecho()
    curses.curs_set(0)
    curses.start_color()
    if tenths is not None:
        curses.halfdelay(tenths)
    curses.init_pair(1, curses.COLOR_RED, curses.COLOR_BLACK)
    curses.init_pair(2, curses.COLOR_YELLOW, curses.COLOR_BLACK)
    curses.init_pair(3, curses.COLOR_GREEN, curses.COLOR_BLACK)
    curses.init_pair(4, curses.COLOR_CYAN, curses.COLOR_BLACK)
    curses.init_pair(5, curses.COLOR_BLUE, curses.COLOR_BLACK)
    curses.init_pair(6, curses.COLOR_MAGENTA, curses.COLOR_BLACK)
    curses.init_pair(7, curses.COLOR_WHITE, curses.COLOR_BLACK)
    return [VAR_SCR, stdscr]

def ef_nc_getmaxyx(*args):
    fn = "nc-getmaxyx"
    fargc_must_eq(fn, args, 1)
    fargt_must_eq(fn, args, 0, VAR_SCR)
    stdscr = args[0][1]
    y, x = stdscr.getmaxyx()
    return [VAR_LIST, [[VAR_NUM, y], [VAR_NUM, x]]]

def ef_nc_addstr(*args):
    fn = "nc-addstr"
    fargc_must_ge(fn, args, 4)
    fargt_must_eq(fn, args, 0, VAR_SCR)
    fargt_must_eq(fn, args, 1, VAR_NUM)
    fargt_must_eq(fn, args, 2, VAR_NUM)
    fargt_must_eq(fn, args, 3, VAR_STRING)
    if len(args) >= 5:
        fargt_must_eq(fn, args, 4, VAR_NUM)
        c = args[4][1]
    else:
        c = 7
    stdscr = args[0][1]
    y = args[1][1]
    x = args[2][1]
    s = args[3][1]
    stdscr.addstr(y, x, s, curses.color_pair(c))
    return [VAR_VOID]

def ef_nc_getch(*args):
    fn = "nc-getch"
    fargc_must_eq(fn, args, 1)
    fargt_must_eq(fn, args, 0, VAR_SCR)
    stdscr = args[0][1]
    c = stdscr.getch()
    if c == curses.KEY_RESIZE:
        ef_nc_endwin()
        sys.exit(1)
    return [VAR_NUM, c]

def ef_nc_endwin(*args):
    fn = "nc-endwin"
    fargc_must_eq(fn, args, 0)
    curses.endwin()
    return [VAR_VOID]

efs_nc = [
        ("nc-initscr", ef_nc_initscr),
        ("nc-getmaxyx", ef_nc_getmaxyx),
        ("nc-addstr", ef_nc_addstr),
        ("nc-getch", ef_nc_getch),
        ("nc-endwin", ef_nc_endwin)]

# feature (unused): last successfully parsed text is saved
# aside so that difficulty on dropped parenths can be easily
# reverted when having done a too eager edit.  to be called
# after each successful parse of a filename; also import.
#
# import os, shutil
#
# def ok_parse_bak(f, fn):
#     def get_m(fn):
#         try:
#             return os.stat(fn).st_mtime
#         except FileNotFoundError:
#             return 0
#     txt_m = os.fstat(f.fileno()).st_mtime
#     bak = fn + ".ok"
#     bak_m = get_m(bak)
#     if txt_m > bak_m:
#         shutil.copyfile(fn, bak)

def compxrun(src, names, macros, env, fn):
    try:
        ast = compx(src, names, macros, env.keys())
        run_top(ast, env, names)
    except Exception as e:
        ty = "error"
        if type(e) == SrcError: ty = "src-error"
        elif type(e) == RunError: ty = "run-error"
        if fn: sys.stderr.write("%s in %s,\n" % (ty, fn))
        else: sys.stderr.write("%s,\n" % (ty,))
        sys.stderr.write(str(e.args[0]) + "\n")
        if verbose:
            raise e
    else:
        return True

if __name__ == "__main__":
    command_line = sys.argv[1:]  # skip Humble itself
    class SrcOpener:
        def __init__(self):
            self.filename = None
        def __call__(self, path):
            self.filename = path
            return open(path, "r", encoding="utf-8")
    opener = SrcOpener()
    if len(sys.argv) >= 2:
        if len(sys.argv) >= 3:
            if (sys.argv[1]) != "-v":
                error("unknown option")
            set_verbose(True)
            fn = sys.argv[2]
        else:
            fn = sys.argv[1]
        efs = efs_nc
        names, env, macros = init_top(opener, efs)
        with opener(fn) as f:
            ok = compxrun(f.read(), names, macros, env, opener.filename)
        sys.exit(int(not ok))

    if not sys.stdin.isatty():
        error("missing input file")
        sys.exit(1)
    sys.stdout.write("WELCOME TO HUMBLE SCHEME.\n"
            "End line with ';' to run and with ';;' to show expanded code.\n"
            "Enter a lone ;; on a line to toggle verbose debug.\n"
            "Provide eof indication to exit.\n--\n")
    names, env, macros = init_top(opener)
    buf = []
    while True:
        try:
            line = input(":")
        except EOFError:
            break
        line = line.lstrip(":").rstrip()
        buf.extend([line, "\n"])
        if line.endswith(";"):
            if line.endswith(";;"):
                if not verbose:
                    set_verbose(True)
                    debug("names", list(enumerate(names)))
                else:
                    set_verbose(False)
            compxrun("".join(buf), names, macros, env, opener.filename)
            opener.filename = None
            buf = []
    sys.stdout.write("\nfare well.\n")

