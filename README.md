# Humble Scheme

"mics.py" is an interpreter that may be started in
interactive mode, or run a program from file.

As an example we extend with curses and create the
*SNAKE* game, maintaining a persisted high-score.

Please see the comments in "mics.py" itself for an
introduction to the language.

The beginnings of an implementation in C++ is found
under "src" where "manifest.txt" is the planned layout.

# Compiling the Interpreter

This is a work-in-progress and there is not yet
any full interpreter written in C++.
But you are welcome to hack along,
as a fellow humble programmer.

## Get packages, i-e
```
apt install build-essential cmake libgtest-dev
( cd /usr/src/googletest
  ; sudo cmake .
  ; sudo cmake --build . --target install
)
```
## Build and run tests
```
cmake -S src
cmake --build .
ctest --verbose
```

