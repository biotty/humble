#!/usr/bin/env sh

./Humble.py import_test/main.scm > out.import_test.txt
./Humble.py io_test.scm > out.io_test.txt
./Humble.py test.scm > out.test.txt

cmake -S src && cmake --build . && ctest # --verbose
head -n 220 test.scm > hest.scm
./humble hest.scm > out.hest.txt

