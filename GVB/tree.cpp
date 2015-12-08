#include <sstream>
#include "tree.h"

long O::n = 0;

int Func::getFuncType(int f) { //函数返回值类型
    switch (f) {
    case CHR: case STR: case MKS: case MKI: case LEFT: case MID: case RIGHT:
        return VSTRING;
    default:
        return VREAL;
    }
}

int Func::getFuncParamType(int f, int i) { //函数参数类型
    switch (f) {
    case ASC: case LEN: case CVI: case CVS: case VAL:
        return VSTRING;
    case LEFT: case RIGHT: case MID:
        if (i == 0) {
            return VSTRING;
        }
    default:
        return VREAL;
    }
}

string E::typeDesc(int t) {
    switch (t) {
    case E::VREAL:
        return "real";
    case E::VINT:
        return "int";
    case E::VSTRING:
        return "string";
    default:
        {
            std::stringstream s;
            s << "unknown type: " << t;
            return s.str();
        }
    }
}