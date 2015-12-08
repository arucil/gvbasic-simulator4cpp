#include "util.h"

size_t StringHash::operator()(const string &v) const {
    int l = v.length();
    long h = 0;
    for (int i = 0; i < l; i++)
        h = h * -1664117991L + (v[i] & 0xff);
    return h;
}

bool StringHash::operator()(const string &v1, const string &v2) const {
    return v1.compare(v2) < 0;
}