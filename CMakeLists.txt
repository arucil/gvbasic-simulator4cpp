cmake_minimum_required(VERSION 3.6)
project(GVBASIC)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        gui_win32/asm.cpp
        gui_win32/custom.cpp
        gui_win32/gvbsim.cpp
        gui_win32/gvbsim.h
        gui_win32/ime.cpp
        gui_win32/ime.h
        gui_win32/resource.h
        GVB/condev.cpp
        GVB/condev.h
        GVB/debug.cpp
        GVB/debug.h
        GVB/error.cpp
        GVB/error.h
        GVB/file.cpp
        GVB/file.h
        GVB/gc.cpp
        GVB/gvb.h
        GVB/gvbc.cpp
        GVB/gvbrun.cpp
        GVB/hash.cpp
        GVB/lex.cpp
        GVB/lex.h
        GVB/replace.cpp
        GVB/test.cpp
        GVB/tree.cpp
        GVB/tree.h
        GVB/util.h)

add_executable(GVBASIC ${SOURCE_FILES})