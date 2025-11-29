# Humble Scheme

"humble.py" is an interpreter that may be started in
interactive mode, or run a program from file.

As an example I extend with "ncurses" and create the
*SNAKE* game, maintaining a persisted high-score.

Please see the comments in "humble.py" itself for an
introduction to the language.  And "test\_humble.py".

The beginnings of an implementation in C++ is found
under "src" where "manifest.txt" is the planned layout.

# Compiling the Interpreter

This is a work-in-progress and there is not yet
any full interpreter.
Here is how I have this development set up.

## Get c++ environment and cmake
```
apt install build-essential cmake libgtest-dev
( cd /usr/src/googletest
  ; sudo cmake .
  ; sudo cmake --build . --target install
)
```
## Build and run tests
```
sh all.sh
```

