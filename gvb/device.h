#ifndef GVBASIC_DEVICE_H
#define GVBASIC_DEVICE_H

#include <string>
#include <cstdint>

namespace gvbsim {

struct Device {
   enum class ScreenMode {
      TEXT, GRAPH
   };

   enum DrawMode {
      CLEAR, PAINT, XOR
   };

   typedef std::uint8_t Coord;
   typedef std::uint8_t byte;
   typedef std::uint16_t Address;

   virtual void appendText(const std::string &) = 0;
   virtual void nextRow() = 0; //如果滚屏则屏幕上滚一行
   virtual void updateLCD() = 0;
   virtual void locate(byte row, byte col) = 0;
   virtual int getX() = 0;
   virtual int getY() = 0;
   virtual void setMode(ScreenMode mode) = 0;
   virtual void cls() = 0;
   virtual std::string input(const std::string &prompt) = 0;
   virtual byte getKey() = 0;
   virtual void point(Coord x, Coord y, DrawMode) = 0;
   virtual void rectangle(Coord x1, Coord y1, Coord x2, Coord y2, bool fill, DrawMode) = 0;
   virtual void line(Coord x1, Coord y1, Coord x2, Coord y2, DrawMode) = 0;
   virtual void ellipse(Coord x, Coord y, Coord rx, Coord ry, bool fill, DrawMode) = 0;

   virtual byte peek(Address) = 0;
   virtual void poke(Address, byte value) = 0;
   virtual void call(Address) = 0;

   virtual void sleep(int ticks) = 0;
};

}

#endif //GVBASIC_DEVICE_H
