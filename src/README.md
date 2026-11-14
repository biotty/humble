# Welcome to Humble Scheme

[r7rs](https://standards.scheme.org/official/r7rs.pdf)
minus call/cc, multi-value, parametrize, exception-handler,
define-syntax and pluss macro, with-pipe, import and a
slight deviation in set!
How is that for an intro?

Of-course there is full tail-call optimization, TCO.
My scheme is more humble than yours!

# Documentation

Should be explorable, and you have "Test.scm" and
other examples.  To embedd the interpreter go see
"main.cpp" as example.

# Extensibility

I/O functions were added as extended variable-types, as can
readily be done for a float-type (null destructor means
copyable under set!).  The interpreter may be started after
adding any set of functions or (language-)macros.

Facility to compile a C++ program into a shared-object to be
loaded from the interpreter is provided.  It is exemplified
by the curses module, and loaded in the snake game.

# Index

Overview of names of (hpp and cpp) files.

* except  // toggle throw or exit
* debug  // controls for development
* trace  // breadcrumbs shown on err
* utf  // utf-8 string operations
* tok  // the scanner and tokenizer
* parse  // parser, quote/macro-expand
  * macros  // the macros (notably macro)
* compx  // binding and zloc
* vars  // runtime objects
* cons  // cons type
* eval  // the runtime engine
  * functions  // builtin functions
  * io\_functions  // port
  * fun\_impl  // function support
* xdl // extensions loader
* top  // toplevel
* test_  // unit-tests
  * tok
  * parse
  * vars
  * compx
  * cons
  * eval
* main  // command using library
* Test.scm  // self-test
* snake.scm  // example game

