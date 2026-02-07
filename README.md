# Humble Scheme

`humble` is an interpreter that may be started in
interactive mode, or run a program from file.

The implementation in C++ is found under [src](src/) where
there are no dependencies for the interpreter itself:
Not even the C++ standard "algorithm" is used, and it
should be trivial to build without exceptions if that
is desired.  I use the 2023 revision of C++ language.
Accomodation for systems that are not POSIX compliant
has not been provided.

The "curses" extension is added as an example and if
not wanted that so-module target as well as `-fPIC`
may be dropped from the build-instructions.  But then there
is no game:  As an example I create the
*SNAKE* game, maintaining a persisted high-score,
[snake.scm](snake.scm).

The interpreter was implemented in Python3 initially
and side-by-side the C++ implementation.
All tests result equal between the implementations;
[test.scm](test.scm), [import\_test](import\_test/)
and [io\_test.scm](io\_test.scm).
The Python implementation is not suggested for other
usage than as a reference.  To run it
you only need Python3 and a very few modules
that come in a normal Python package.
Please see "Humble.py" for some comments on
the concepts of the interpreter and the language.
You will recognize it as Scheme.

The language is a subset of r7rs except that variables
are not "duplicated" implicitly when evaluated.
The usual "define" form however, *does*, and I have "ref"
as the counter-part without.  By "duplicated" I mean
that when more than one reference is held to a value,
a new variable will be created with a copy of that value.
What you will observe is that when passing paramenters,
set! will affect the variable used as argument.  This is
an effect of the non-duplicating evaluation mentioned.
The omission of the implicit duplication
permits benefit of an optimization regarding the memory
layout of lists:  Lists are stored in a contiguous
array until they cannot be, and they are converted to
"cons" chains as usual in Scheme or LISPS in general.
The reason it cannot be stored as a contiguous array
is that ownership of elements gets shared:
If a reference to a cons-cell is given out, i-e with
the "cdr" function, or that a reference to the very
list is given out, such as "duplicating" the variable,
then I convert the list to a cons-chain.

That was a mouthful.  But the idea is quite simple,
and is the main deviation from standard Scheme.

Other differences are either parts in Scheme r7rs that
are absent, or that are added, such as the existence
of an at-operator that works separately as well as
together as the standard comma-at operator.  I also
have the user "macro" mechanism back from LISP.
Please note that the only numeric type is the integer.

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
