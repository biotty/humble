cmake_minimum_required(VERSION 3.14)
project(humble CXX)
set(CMAKE_CXX_STANDARD 17)
enable_testing()
find_package(GTest REQUIRED)
add_executable(test_tok test_tok.cpp tok.cpp)
target_link_libraries(test_tok GTest::GTest GTest::Main)
add_test(tok test_tok)

