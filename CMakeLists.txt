cmake_minimum_required(VERSION 3.6)
project(GVBASIC)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-operator-names")

set(GVB_SOURCE_FILES
        gvb/lex.h
        gvb/lex.cpp
        gvb/tree.h
        gvb/value.h
        gvb/random.h
        gvb/real.h
        gvb/real.cpp
        gvb/gvb.h
        gvb/node_man.h
        gvb/node.h
        gvb/node_man.cpp
        gvb/compile.h
        gvb/compile.cpp
        gvb/error.h
        gvb/gvb.cpp
        gvb/value.cpp
        gvb/data_man.h
        gvb/data_man.cpp
        gvb/func.h
        gvb/func.cpp
        gvb/test/test_parse.cpp
        gvb/test/debug.h
        gvb/test/debug.cpp
        gvb/device.h
        gvb/runtime.cpp
        gvb/file.h
        gvb/file.cpp
        gvb/igui.h
        # gvb/test/test_runtime.cpp
        gvb/device.cpp
        gvb/fake6502_wrap.cpp
        )

add_executable(GVBASIC ${GVB_SOURCE_FILES})
