cmake_minimum_required(VERSION 3.6)
project(GVBASIC)

set(CMAKE_CXX_STANDARD 14)

set(SOURCE_FILES gvb/lex.h
        gvb/lex.cpp
        gvb/tree.h
        gvb/value.h
        gvb/random.h
        gvb/real.h
        gvb/real.cpp
        gvb/gvb.h gvb/node_mgr.h gvb/node.h gvb/node_mgr.cpp gvb/compile.h gvb/compile.cpp gvb/error.h gvb/gvb.cpp gvb/value.cpp)

add_executable(GVBASIC ${SOURCE_FILES})