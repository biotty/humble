
./humble.py import_test/main.hum > out.import_test.txt
./humble.py io_test.hum > out.io_test.txt
./test_humble.py > out.test_humble.txt

cmake -S src && cmake --build . && ctest --verbose

