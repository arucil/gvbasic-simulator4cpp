#pragma once
#include "gvb.h"

struct Device0 : Device {
    void appendText(const string &);
    void nextLine();
    void updateLCD() {}
    void locate(int col, int row) {}
    int getX() {return 0;}
    int getY() {return 0;}
    void setMode(int mode) {}
    int getkey();
    void cls() {}
    string input(const string &prompt, int type);
    void point(coord x, coord y, int mode) {}
    void rectangle(coord x1, coord y1, coord x2, coord y2, bool fill, int mode);
    void line(coord x1, coord y1, coord x2, coord y2, int mode) {}
    void ellipse(coord x, coord y, coord rx, coord ry, bool fill, int mode) {}

    int peek(int addr) {return 0;}
    string peek(int,int){return "";}
    void poke(int addr, byte value){}
    void poke(int,const char *,int){}
    void call(int addr){}

    void sleep(int) {}
    void paint(int,int,int,byte,byte,int) {}
    bool getPoint(int,int){return true;}
    bool checkKey(int){return false;}
};