cmake_minimum_required(VERSION 3.14)
project(humble CXX)
set(CMAKE_CXX_STANDARD 23)
enable_testing()
add_compile_options(-ggdb -Wall -Wextra -Werror)
find_package(GTest REQUIRED)

add_executable(test_tok test_tok.cpp tok.cpp utf.cpp)
target_link_libraries(test_tok GTest::GTest GTest::Main)
add_test(tok test_tok)

add_executable(test_utf test_utf.cpp utf.cpp)
target_link_libraries(test_utf GTest::GTest GTest::Main)
add_test(utf test_utf)
