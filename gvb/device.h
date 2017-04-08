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

public:
   Device();

public:
   void appendText(const std::string &);
   void nextRow(); //如果滚屏则屏幕上滚一行
   void updateLCD();
   void locate(uint8_t row, uint8_t col);
   uint8_t getX();
   uint8_t getY();
   void setMode(ScreenMode mode);
   void cls();
   std::string input();
   uint8_t getKey();
   void point(uint8_t x, uint8_t y, DrawMode);
   void rectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool fill, DrawMode);
   void line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, DrawMode);
   void ellipse(uint8_t x, uint8_t y, uint8_t rx, uint8_t ry, bool fill, DrawMode);

   uint8_t peek(uint16_t);
   void poke(uint16_t, uint8_t value);
   void call(uint16_t);

   void sleep(int ticks);
};

}

#endif //GVBASIC_DEVICE_H
