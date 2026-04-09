#!/usr/bin/env bash

./Humble.py test.scm > out.a-test.txt
./Humble.py import_test/main.scm > out.a-import_test.txt
./Humble.py io_test.scm > out.a-io_test.txt

cmake -S src && cmake --build . && ctest # --verbose

./humble test.scm > out.b-test.txt
./humble import_test/main.scm > out.b-import_test.txt
./humble io_test.scm > out.b-io_test.txt

check(){
  sed -E 's/&[0-9][0-9]*/SYM/g' < $1 > $1.nosym
  sed -E 's/&[0-9][0-9]*/SYM/g' < $2 > $2.nosym
  S=$(diff -u $1.nosym $2.nosym)
  if [ -n "$S" ]
  then
    echo "$S"
    exit 1
  fi
}

check out.{a,b}-test.txt
check out.{a,b}-import_test.txt
check out.{a,b}-io_test.txt

cmake --install .
echo
echo "You're invited to play the snake game with the following command"
echo "Please note that you navigate with the keys [A] [Z] [O] and [P]."
echo
echo "HUMBLE_DIR=$HOME/.local/humble $HOME/.local/bin/humble snake.scm"

