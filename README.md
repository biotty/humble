# Humble Scheme

"Humble.py" is an interpreter that may be started in
interactive mode, or run a program from file.

As an example I extend with "ncurses" and create the
*SNAKE* game, maintaining a persisted high-score.

Please see the comments in "Humble.py" itself for an
introduction to the language.  And "test.scm".

The beginnings of an implementation in C++ is found
under "src" where "manifest.txt" describes layout.

# Compiling the Interpreter

Here is how I have this development set up.

## Build and run tests
```
sh all.sh
```

## C++ environment help
```
sudo apt install build-essential cmake libgtest-dev
( cd /usr/src/googletest
  ; sudo cmake .
  ; sudo cmake --build . --target install
)
```
