#include <iostream>
#include "condev.h"

using namespace std;

void Device0::appendText(const string &s) {
    cout << s;
}

void Device0::nextLine() {
    cout << endl;
}

int Device0::getkey() {
    return cin.get();
}

string Device0::input(const string &prompt, int n) {
    string s;
    cin>>s;
    return s;
}

void Device0::rectangle(coord x1, coord y1, coord x2, coord y2, bool fill, int mode) {
    cout<<"box "<<(int)x1<<","<<(int)y1<<","<<(int)x2<<","<<(int)y2<<endl;
}