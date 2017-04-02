#pragma once

#include <exception>
#include <string>

using std::exception;
using std::string;

struct GVBError : exception {
    int line, label;

    GVBError(int line, int label, const char *s)
        : exception(s), line(line), label(label) {}
    GVBError(int line, int label, const string &s)
        : exception(s.c_str()), line(line), label(label) {}
    virtual string description() const;
};

typedef GVBError CE, RE;