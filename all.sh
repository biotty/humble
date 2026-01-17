#!/usr/bin/env sh

./humble.py import_test/main.scm > out.import_test.txt
./humble.py io_test.scm > out.io_test.txt
./humble.py test.scm > out.test.txt

cmake -S src && cmake --build . && ctest # --verbose
head -n 218 test.scm > hest.scm
./humble hest.scm > out.hest.txt

