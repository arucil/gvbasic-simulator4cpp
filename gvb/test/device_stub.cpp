#include "../device.h"
#include <iostream>


using namespace std;
using namespace gvbsim;

Device::Device() { }


void Device::appendText(const std::string &s) {
   cout << s;
}

void Device::nextRow() {
   cout << endl;
}

Device::byte Device::getKey() {
   return cin.get();
}

string Device::input() {
   string s;
   cin >> s;
   return s;
}

void Device::updateLCD() {

}

void Device::locate(byte row, byte col) { }

int Device::getX() { return 0; }

int Device::getY() { return 0; }

void Device::setMode(ScreenMode mode) { }

void Device::cls() { }

void Device::point(Coord x, Coord y, DrawMode) { }

void Device::rectangle(Coord x1, Coord y1, Coord x2, Coord y2, bool fill,
                       DrawMode) { }

void Device::line(Coord x1, Coord y1, Coord x2, Coord y2, DrawMode) { }

void Device::ellipse(Coord x, Coord y, Coord rx, Coord ry, bool fill,
                     DrawMode) { }

Device::byte Device::peek(Address) { return 0; }

void Device::poke(Address, byte value) { }

void Device::call(Address) { }

void Device::sleep(int ticks) { }