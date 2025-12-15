cmake_minimum_required(VERSION 3.14)
project(humble CXX)
set(CMAKE_CXX_STANDARD 23)
enable_testing()
add_compile_options(-Wall -Wextra -Werror)
#add_compile_options(-DDEBUG)
add_compile_options(-ggdb)
add_compile_options(-fsanitize=address)

find_package(GTest REQUIRED)

add_library(humble
    xeval.cpp
    vars.cpp
    compx.cpp
    parse.cpp
    tok.cpp
    utf.cpp
    debug.cpp)

add_executable(test_utf test_utf.cpp)
target_link_libraries(test_utf GTest::GTest GTest::Main humble -lasan)
add_test(utf test_utf)

add_executable(test_tok test_tok.cpp)
target_link_libraries(test_tok GTest::GTest GTest::Main humble -lasan)
add_test(tok test_tok)

add_executable(test_parse test_parse.cpp)
target_link_libraries(test_parse GTest::GTest GTest::Main humble -lasan)
add_test(parse test_parse)

add_executable(test_compx test_compx.cpp)
target_link_libraries(test_compx GTest::GTest GTest::Main humble -lasan)
add_test(compx test_compx)

add_executable(test_vars test_vars.cpp)
target_link_libraries(test_vars GTest::GTest GTest::Main humble -lasan)
add_test(vars test_vars)

add_executable(test_xeval test_xeval.cpp)
target_link_libraries(test_xeval GTest::GTest GTest::Main humble -lasan)
add_test(xeval test_xeval)

