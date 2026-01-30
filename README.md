# Humble Scheme

"Humble.py" is an interpreter that may be started in
interactive mode, or run a program from file.

As an example I extend with "ncurses" and create the
*SNAKE* game, maintaining a persisted high-score.
You only need python3 and a very few standard modules
that comes with python.

Please see the comments in "Humble.py" itself for an
introduction to the language.  If you know Scheme
you will recognize a subset of r7rs.
The interpreter has been implemented in C++ with all
test-results equal between the implementations;
"test.scm", "import\_test/", "io\_test.scm".
For notes specific to the C++
implementation see [src](src/README.md).

There is one subtle deviation from usual scheme
semantics, namely that set! operate as on lvalue
instead of on environment.  Also, these lvalues are
passed as lambda-parameters without copying them
(as can be done by the "dup" or the "local" macro).
Lists are contiguous till cdr-used (or a reference to
it shared):  This is a significant efficiency-gain
that would not have effect if we implicitly "dup"
at each parameter-pass as done in traditional scheme.
"define" will "dup" (of-course no copy if sole ref),
to meet conformance to scheme, and the alternative
(copy-less) is named "ref".

The implementation in C++ is found under "src" where
"manifest.txt" describes layout.  There are no
dependencies for the interpreter itself, but you may
embed it and thus extend with your own functions.
Not even the C++ standard "algorithm" is used, and it
should be trivial to build without exceptions, if that
is desired.

The curses extension is added as an example, and if
not desired, that so-module target as well as -fPIC
may be dropped from the build.  But then there is
no snake game.

# Compiling the Interpreter

Here is how I have this development set up.

## Build and run tests
```
sh all.sh
```

## C++ environment help
```
sudo apt install build-essential cmake libgtest-dev \
                 curses-dev  # for snake-game example
( cd /usr/src/googletest
  ; sudo cmake .
  ; sudo cmake --build . --target install
)
```
