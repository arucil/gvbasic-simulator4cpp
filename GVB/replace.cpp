#include <cstring>
#include "util.h"

using std::strlen;

string replaceAll(const string &str, const char *s1, const char *s2) {
    string s(str);
    int l1 = strlen(s1) - 1, l2 = strlen(s2);

    for (int i = s.length() - 1, j; i >= 0; i--) {
        if (s[i] == s1[l1]) {
            for (j = l1 - 1, i--; j >= 0 && i >= 0 && s[i] == s1[j]; j--, i--);
            if (j < 0) {
                s.replace(++i, l1 + 1, s2, 0, l2);
            }
        }
    }
    return s;
}

string replaceAll(const char *str, const char *s1, const char *s2) {
    string s(str);
    int l1 = strlen(s1) - 1, l2 = strlen(s2);

    for (int i = s.length() - 1, j; i >= 0; i--) {
        if (s[i] == s1[l1]) {
            for (j = l1 - 1, i--; j >= 0 && i >= 0 && s[i] == s1[j]; j--, i--);
            if (j < 0) {
                s.replace(++i, l1 + 1, s2, 0, l2);
            }
        }
    }
    return s;
}