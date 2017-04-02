cmake_minimum_required(VERSION 3.6)
project(GVBASIC)

set(CMAKE_CXX_STANDARD 14)

set(SOURCE_FILES gvb/lex.h gvb/common.h gvb/lex.cpp gvb/test/test_lex.cpp)

add_executable(GVBASIC ${SOURCE_FILES})