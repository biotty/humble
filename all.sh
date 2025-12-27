#!/usr/bin/env sh

./humble.py import_test/main.scm > out.import_test.txt
./humble.py io_test.scm > out.io_test.txt
./test_humble.py > out.test_humble.txt

cmake -S src && cmake --build . && ctest --verbose

