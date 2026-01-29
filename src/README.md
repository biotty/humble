# Welcome to Humble Scheme

[r7rs](https://standards.scheme.org/official/r7rs.pdf)
minus call/cc, multi-value, parametrize, exception-handler,
define-syntax and pluss macro, with-pipe, import and a
slight deviation in set!
How is that for an intro?

Of-course there is full tail-call optimization, TCO.

There are macros but in general good extensibility
and modularity.

My scheme is more humble than yours!

# Documentation

Should be explorable, and you have "test.scm" and
other examples.  To embedd the interpreter go see
"main.cpp" as example.

# Notes

Should be thread-safe as the few globals are not
mutated once setup.

# Further study

To implement call/cc one could use c++ co-routines or thread
(we want to save the stack and get back to it, right?).

# Extensibility

I/O functions were added as extended variable-types, as can
readily be done for a float-type (null destructor means
copyable under set!).  The interpreter may be started after
adding any set of functions or (language-)macros.

# Index

Overview of names of (hpp and cpp) files.

* except  // for users of library
* debug  // controls for development
* utf  // utf-8 string operations
* tok  // the scanner and tokenizer
* parse  // parser, quote/macro-expand
  * macros  // the macros (and 'macro')
* compx  // binding and zloc
* vars  // runtime objects
* cons  // cons type
* xeval  // the runtime engine
  * functions  // builtin functions
  * io_functions  // port
  * dlopen_funtions // extensions
* top  // toplevel
* test_  // unit-tests
  * tok
  * parse
  * vars
  * compx
  * cons
  * xeval
* main  // command using library
* test.scm  // self-test
* snake.scm  // example game

