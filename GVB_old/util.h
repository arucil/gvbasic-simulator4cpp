#pragma once

#include <string>

using std::string;

class StringHash { //用于hash_map<string, *>的hash函数：hash_map<string, *, StringHash>
public:
    enum {
        bucket_size = 17
    };
    size_t operator()(const string &v) const;
    bool operator()(const string &v1, const string &v2) const;
};

string replaceAll(const string &s, const char *s1, const char *s2);
string replaceAll(const char *s, const char *s1, const char *s2);