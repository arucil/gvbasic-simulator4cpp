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

uint8_t Device::getKey() {
   return cin.get();
}

string Device::input() {
   string s;
   cin >> s;
   return s;
}

void Device::updateLCD() {

}

void Device::locate(uint8_t row, uint8_t col) { }

uint8_t Device::getX() { return 1; }

uint8_t Device::getY() { return 0; }

void Device::setMode(ScreenMode mode) { }

void Device::cls() { }

void Device::point(uint8_t x, uint8_t y, DrawMode) { }

void Device::rectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool fill,
                       DrawMode) { }

void Device::line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, DrawMode) { }

void Device::ellipse(uint8_t x, uint8_t y, uint8_t rx, uint8_t ry, bool fill,
                     DrawMode) { }

uint8_t Device::peek(uint16_t) { return 0; }

void Device::poke(uint16_t, uint8_t value) { }

void Device::call(uint16_t) { }

void Device::sleep(int ticks) { }