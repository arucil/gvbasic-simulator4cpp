#ifndef GVBASIC_DEVICE_H
#define GVBASIC_DEVICE_H

#include <string>
#include <cstdint>

namespace gvbsim {


class Device {
public:
   enum class ScreenMode {
      TEXT, GRAPH
   };

   enum DrawMode {
      CLEAR, PAINT, XOR
   };

   typedef std::uint8_t Coord;
   typedef std::uint8_t byte;
   typedef std::uint16_t Address;

public:
   Device();

public:
   void appendText(const std::string &);
   void nextRow(); //如果滚屏则屏幕上滚一行
   void updateLCD();
   void locate(byte row, byte col);
   int getX();
   int getY();
   void setMode(ScreenMode mode);
   void cls();
   std::string input();
   byte getKey();
   void point(Coord x, Coord y, DrawMode);
   void rectangle(Coord x1, Coord y1, Coord x2, Coord y2, bool fill, DrawMode);
   void line(Coord x1, Coord y1, Coord x2, Coord y2, DrawMode);
   void ellipse(Coord x, Coord y, Coord rx, Coord ry, bool fill, DrawMode);

   byte peek(Address);
   void poke(Address, byte value);
   void call(Address);

   void sleep(int ticks);
};

}

#endif //GVBASIC_DEVICE_H
