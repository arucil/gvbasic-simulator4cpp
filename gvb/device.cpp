#include "device.h"
#include <cstring>
#include <cstdio>
#include <algorithm>
#include "igui.h"
#include "gvb.h"

using namespace std;
using namespace gvbsim;


uint8_t Device::s_dataImage[16864];
uint8_t Device::s_dataAscii[2560];
uint8_t Device::s_dataGB16[243648];
uint8_t Device::s_dataGB12[182736];
uint8_t Device::s_dataGBChar[14696];
uint8_t Device::s_dataPY[4576];

const uint8_t Device::s_bitmask[8] = {
      128, 64, 32, 16, 8, 4, 2, 1
};


Device::Device() : m_gui(nullptr) {
}

void Device::setGui(IGui *gui) {
   m_gui = gui;
}

void Device::loadData() {
   loadFile("res/ascii16.bin", s_dataAscii, sizeof s_dataAscii);
   loadFile("res/image16.bin", s_dataImage, sizeof s_dataImage);
   loadFile("res/gbfont16.bin", s_dataGB16, sizeof s_dataGB16);
   loadFile("res/gbfont12.bin", s_dataGB12, sizeof s_dataGB12);
   loadFile("res/gb.dat", s_dataGBChar, sizeof s_dataGBChar);
   loadFile("res/py.dat", s_dataPY, sizeof s_dataPY);
}

inline void Device::loadFile(const char *fn, uint8_t *p, size_t n) {
   FILE *fp = fopen(fn, "rb");
   if (!fp)
      return;
   fread(p, n, 1, fp);
   fclose(fp);
}

void Device::setGraphAddr(uint16_t addr) {
   m_memGraph = m_mem + (m_addrGraph = addr);
}

void Device::setTextAddr(uint16_t addr) {
   m_memText = m_mem + (m_addrText = addr);
}

void Device::setKeyAddr(uint16_t addr) {
   m_memKey = m_mem + (m_addrKey = addr);
}

void Device::setKeyMapAddr(uint16_t addr) {
   m_memKeyMap = m_mem + (m_addrKeyMap = addr);
}

void Device::moveRow() {
   lock_guard<mutex> lock(m_mutGraph);
   memmove(m_memText, m_memText + 20, 80);
   memset(m_memText + 80, 0, 20);
   m_y--;
   memmove(m_memGraph, m_memGraph + 320, 1600 - 320);
   memset(m_memGraph + 1600 - 320, 0, 320);
}

void Device::appendText(const std::string &s) {
   for (size_t i = 0; i < s.size(); i++) {
      if (static_cast<uint8_t>(s[i]) > 160 && m_x == 19) {
         m_memText[m_y * 20 + 19] = 32;
         m_x = 0;
         m_y++;
      }
      if (m_y >= 5) {
         moveRow();
      }
      m_memText[m_y * 20 + m_x++] = static_cast<uint8_t>(s[i]);
      if (static_cast<uint8_t>(s[i]) > 160 && i < s.size() - 1) {
         m_memText[m_y * 20 + m_x++] = static_cast<uint8_t>(s[++i]);
      }
      if (m_x >= 20) {
         m_y++;
         m_x = 0;
      }
   }

   int i;
   for (i = m_x; i < 20; i++) {
      if (m_memText[m_y * 20 + i]) {
         break;
      }
   }
   uint8_t *t = m_memText + m_y * 20 + i;
   for (; i < 20; i++) {
      if (!*t) {
         break;
      }
      *t++ = 0;
   }
   if (m_y >= 5) {
      moveRow();
   }
}

void Device::nextRow() {
   m_x = 0;
   ++m_y;
   if (m_y >= 5) {
      moveRow();
   }
}

void Device::updateLCD() {
   {
      lock_guard<mutex> lock(m_mutGraph);

      if (m_scrMode == ScreenMode::TEXT) {
         memset(m_memGraph, 0, 1600);
      }
      uint8_t *c = m_memText, *f, *g, t; //f字库 g屏幕缓冲
      long ff;

      for (int j = 0; j < 5; j++) {
         for (int i = 0; i < 20; i++) {
            g = j * 320 + i + m_memGraph;
            if (*c > 160) {
               ff = (*c << 8) | (t = c[1]);
               if (ff >= 0xf8a1 && ff <= 0xfdd9 && t > 160) { //wqx图形
                  f = s_dataImage + (((*c - 248) * 94 + t - 161) << 5);
               } else {
                  ff = *c - 161;
                  if (ff > 8)
                     ff -= 6;
                  ff = (ff * 94 + t - 161) << 5;
                  if (ff < 0 || ff + 32 > sizeof s_dataGB16) {
                     ff = 85 << 5; //黑方块
                  }
                  f = s_dataGB16 + ff;
               }
               if (i++ != 19) {
                  for (int k = 0; k < 16; k++) {
                     *g = *f++;
                     g[1] = *f++;
                     g += 20;
                  }
               } else { //汉字位于行尾时分成两半显示...
                  for (int k = 0; k < 16; k++) {
                     *g = *f++;
                     g[301] = *f++;
                     g += 20;
                  }
                  j++;
                  i = 0;
               }
               c++;
            } else if (*c) {
               f = *c * 16 + s_dataAscii;
               for (int k = 0; k < 16; k++) {
                  *g = *f++;
                  g += 20;
               }
            }
            c++;
         }
      }
   } // release lock

   m_gui->update();
}

void Device::locate(uint8_t row, uint8_t col) {
   m_x = col;
   m_y = row;
   if (ScreenMode::TEXT == m_scrMode) {
      m_gui->update();
   }
}

uint8_t Device::getX() {
   return m_x;
}

uint8_t Device::getY() {
   return m_y;
}

Device::CursorPosInfo Device::getPosInfo() const {
   uint8_t *j = m_memText + m_y * 20;
   CursorPosInfo info = CursorPosInfo::ASCII;

   for (int i = 0; i < m_x; ++i) {
      if (j[i] > 160) {
         if (info == CursorPosInfo::GB1st)
            info = CursorPosInfo::GB2nd;
         else
            info = CursorPosInfo::GB1st;
      } else
         info = CursorPosInfo::ASCII;
   }

   return info;
}

void Device::setMode(ScreenMode mode) {
   if (ScreenMode::TEXT == (m_scrMode = mode)) {
      m_gui->m_displayCursor = false;
   }
   cls();
}

void Device::cls() {
   {
      lock_guard<mutex> lock2(m_mutGraph);

      memset(m_memText, 0, 100);
      memset(m_memGraph, 0, 1600);
      m_x = m_y = 0;
   }

   m_gui->update();
}

string Device::input() {
   return "";
}

uint8_t Device::getKey() {
   m_enableCursor = true;
   int a;
   do {
      lock_guard<mutex> lock(m_mutKey);
      a = static_cast<int>(*m_memKey) - 128;
   } while (a < 0);
   m_enableCursor = false;
   m_gui->m_displayCursor = false;
   {
      lock_guard<mutex> lock(m_mutKey);
      *m_memKey &= 127;
   }
   return static_cast<uint8_t>(a);
}

inline void Device::setPoint(uint8_t x, uint8_t y, DrawMode mode) {
   if (x > 159 || y > 79) {
      return;
   }
   uint8_t *o = m_memGraph + y * 20 + (x >> 3);
   switch (mode) {
   case PAINT:
      *o |= s_bitmask[x & 7];
      break;
   case CLEAR:
      *o &= ~s_bitmask[x & 7];
      break;
   case XOR:
      *o ^= s_bitmask[x & 7];
      break;
   default:
      break;
   }
}

void Device::point(uint8_t x, uint8_t y, DrawMode mode) {
   {
      lock_guard<mutex> lock(m_mutGraph);
      setPoint(x, y, mode);
   }
   m_gui->update(x, y, x, y);
}

inline void Device::hLine(uint8_t x1, uint8_t x2, uint8_t y, DrawMode mode) {
   while (x1 <= x2) {
      setPoint(x1, y, mode);
      ++x1;
   }
}

void Device::rectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2,
                       bool fill, DrawMode mode) {
   int y1_;
   {
      lock_guard<mutex> lock(m_mutGraph);

      if (x1 > x2) {
         auto t = x1;
         x1 = x2;
         x2 = t;
      }
      if (y1 > y2) {
         auto t = y1;
         y1 = y2;
         y2 = t;
      }
      if (x2 > 159)
         x2 = 159;

      if (y2 > 79)
         y2 = 79;

      y1_ = y1;
      if (fill) {
         for (; y1 <= y2; y1++)
            hLine(x1, x2, y1, mode);
      } else {
         hLine(x1, x2, y1, mode);
         hLine(x1, x2, y2, mode);
         for (; y1 <= y2; y1++) {
            setPoint(x1, y1, mode);
            setPoint(x2, y1, mode);
         }
      }
   }
   m_gui->update(x1, y1_, x2, y2);
}

void Device::line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, DrawMode mode) {
   int x_, y_;
   {
      lock_guard<mutex> lock(m_mutGraph);

      if (x1 > x2) {
         auto t = x1;
         x1 = x2;
         x2 = t;
         t = y1;
         y1 = y2;
         y2 = t;
      }

      int dx = x2 - x1, dy = y2 - y1, sgn = 1, tx = 0, ty = 0;
      x_ = x1;
      y_ = y1;
      if (dy < 0) {
         dy = -dy;
         sgn = -1;
      }
      int m = dx > dy ? dx : dy, i = m;
      while (i-- >= 0) {
         setPoint(x1, y1, mode);
         tx += dx;
         ty += dy;
         if (tx >= m) {
            x1++;
            tx -= m;
         }
         if (ty >= m) {
            y1 += sgn;
            ty -= m;
         }
      }
   }
   m_gui->update(x_, min<int>(y_, y2), x2, max<int>(y_, y2));
}

void Device::ovalPoint(uint8_t ox, uint8_t oy, uint8_t x, uint8_t y, DrawMode mode) {
   setPoint(ox - x, oy - y, mode);
   setPoint(ox - x, oy + y, mode);
   setPoint(ox + x, oy - y, mode);
   setPoint(ox + x, oy + y, mode);
}

void Device::ellipse(uint8_t x, uint8_t y, uint8_t rx, uint8_t ry, bool fill,
                     DrawMode mode) {
   {
      lock_guard<mutex> lock(m_mutGraph);

      int asq = rx * rx, bsq = ry * ry;
      int asq2 = asq * 2, bsq2 = bsq * 2;
      int p;
      int x1 = 0, y1 = ry;
      int px = 0, py = asq2 * y1;
      p = bsq - asq * ry + ((asq + 2) >> 2);
      while (px < py) {
         x1++;
         px += bsq2;
         if (p < 0) {
            p += bsq + px;
         } else {
            if (fill) {
               hLine(x - x1 + 1, x + x1 - 1, y + y1, mode);
               hLine(x - x1 + 1, x + x1 - 1, y - y1, mode);
            }
            y1--;
            py -= asq2;
            p += bsq + px - py;
         }
         if (!fill) {
            ovalPoint(x, y, x1, y1, mode);
         }

      }
      if (fill) {
         hLine(x - x1, x + x1, y + y1, mode);
         hLine(x - x1, x + x1, y - y1, mode);
      }
      p = bsq * x1 * x1 + bsq * x1 + asq * (y1 - 1) * (y1 - 1) - asq * bsq + ((bsq + 2) >> 2);
      while (--y1 > 0) {
         py -= asq2;
         if (p > 0) {
            p += asq - py;
         } else {
            x1++;
            px += bsq2;
            p += asq - py + px;
         }
         if (fill) {
            hLine(x - x1, x + x1, y + y1, mode);
            hLine(x - x1, x + x1, y - y1, mode);
         } else {
            ovalPoint(x, y, x1, y1, mode);
         }
      }
      if (fill) {
         hLine(x - rx, x + rx, y, mode);
      } else {
         setPoint(x, y + ry, mode);
         setPoint(x, y - ry, mode);
         setPoint(x + rx, y, mode);
         setPoint(x - rx, y, mode);
      }
   }
   m_gui->update(x - rx, y - ry, x + rx, y + ry);
}

uint8_t Device::peek(uint16_t addr) {
   if (m_addrKey == addr || addr >= m_addrKeyMap && addr < m_addrKeyMap + 8) {
      lock_guard<mutex> lock(m_mutKey);
      return m_mem[addr];
   }
   if (addr >= m_addrGraph && addr < m_addrGraph + 1600) {
      lock_guard<mutex> lock(m_mutGraph);
      return m_mem[addr];
   }
   return m_mem[addr];
}

void Device::poke(uint16_t addr, uint8_t value) {
   if (m_addrKey == addr || addr >= m_addrKeyMap && addr < m_addrKeyMap + 8) {
      lock_guard<mutex> lock(m_mutKey);
      m_mem[addr] = value;
      if (m_addrKey == addr && value == 155)
         throw GVB::Quit();
   } else if (addr >= m_addrGraph && addr < m_addrGraph + 1600) {
      {
         lock_guard<mutex> lock(m_mutGraph);
         m_mem[addr] = value;
      }
      addr -= m_addrGraph;
      int x = addr % 20;
      int y = addr / 20;
      m_gui->update(x << 3, y, x + 1 << 3, y);
   } else {
      m_mem[addr] = value;
   }
}

void Device::sleep(int ticks) {
   m_gui->sleep(ticks);
}

void Device::paint(uint16_t addr, int x, int y, uint8_t w, uint8_t h,
                   PaintMode mode) {
   if (x > 159 || y > 79 || x + w < 0 || y + h < 0) {
      return;
   }

   int h_;
   {
      lock_guard<mutex> lock(m_mutGraph);

      int bw = (w + 7) >> 3;
      //每行开始第一个数据前无用的bit数
      int unuseDataBits = 0;
      if (x < 0) {
         addr += (-x) / 8;
         unuseDataBits = (-x) % 8;
         w += x;
         x = 0;
      }
      if (y < 0) {
         addr += -bw * y;
         h += y;
         y = 0;
      }
      if (x + w > 160) {
         w -= x + w - 160;
      }
      if (y + h > 80) {
         h -= y + h - 80;
      }

      h_ = h;
      //绘制处前无用的bit数
      int unuseScreenBits = x % 8;
      //绘制开始地址
      int offset = 20 * y + x / 8;
      //实际每行用到数据的byte数
      int count = (unuseDataBits + w + 7) / 8;
      //实际绘制影响到的byte数
      int size = (unuseScreenBits + w + 7) / 8;
      //绘制结尾剩下的bit数
      int remain = size * 8 - unuseScreenBits - w;
      //用于存储图像数据
      char mapData[22];
      while (h-- > 0) {
         for (int i = 0; i < count; i++) {
            mapData[i] = peek(addr + i);
         }
         addr += bw;
         rollData(mapData, count + 1, unuseScreenBits - unuseDataBits);
         for (int i = 0; i < size; i++) {
            int s = mapData[i], d = m_memGraph[offset + i];
            int mask = 0;
            if (i == 0) {
               mask |= s_maskr[unuseScreenBits];
            }
            if (i == size - 1) {
               mask |= s_maskl[remain];
            }
            s &= ~mask;
            d &= ~mask;
            m_memGraph[offset + i] &= mask;
            switch (mode) {
            case PaintMode::NOT:
               s ^= ~mask;
               break;
            case PaintMode::OR:
               s |= d;
               break;
            case PaintMode::AND:
               s &= d;
               break;
            case PaintMode::XOR:
               s ^= d;
            }
            m_memGraph[offset + i] |= s;
         }
         offset += 20;
      }
   }
   m_gui->update(x, y, x + w, y + h_);
}


const uint8_t Device::s_maskl[] = {0, 0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f};
const uint8_t Device::s_maskr[] = {0, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe};

// 内存块移位，bit>0右移，<0左移。bit绝对值必须小于8
void Device::rollData(void *data, int size, int bit) {
   if (!bit)
      return;
   uint8_t *d = static_cast<uint8_t *>(data), c, t = 0;
   if (bit > 0) { //右移
      int rb = 8 - bit;
      for (int i = 0; i < size; i++) {
         c = *d;
         *d++ = (c >> bit) | t;
         t = (c << rb) & s_maskr[bit];
      }
   } else { //左移
      bit = -bit;
      int rb = 8 - bit;
      for (int i = 1; i < size; i++) {
         d++;
      }
      for (int i = 0; i < size; i++) {
         c = *d;
         *d-- = (c << bit) | t;
         t = (c >> rb) & s_maskl[bit];
      }
   }
}

void Device::call(uint16_t addr) {
}