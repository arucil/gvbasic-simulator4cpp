#include "device.h"
#include <cstring>
#include <thread>
#include <chrono>
#include "igui.h"
#include "gvb.h"

using namespace std;
using namespace gvbsim;


uint8_t Device::s_dataImage[16864];
uint8_t Device::s_dataAscii16[2560];
uint8_t Device::s_dataGB16[243648];
uint8_t Device::s_dataAscii12[3072];
uint8_t Device::s_dataGB12[182736];
uint8_t Device::s_dataAscii8[1664];
uint8_t Device::s_dataGBChar[14696];
uint8_t Device::s_dataPY[4576];

const uint8_t Device::s_bitmask[8] = {
      128, 64, 32, 16, 8, 4, 2, 1
};


Device::Device() : m_gui(nullptr), m_memGraph(nullptr) {
}

void Device::setGui(IGui *gui) {
   m_gui = gui;
}

void Device::init() {
   m_enableCursor = false;
   *m_memKey = 0;
   memset(m_memKeyMap, 255, 8);
   setMode(ScreenMode::TEXT);
}

void Device::loadData() {
   loadFile("res/ascii16.bin", s_dataAscii16, sizeof s_dataAscii16);
   loadFile("res/image16.bin", s_dataImage, sizeof s_dataImage);
   loadFile("res/gbfont16.bin", s_dataGB16, sizeof s_dataGB16);
   loadFile("res/ascii12.bin", s_dataAscii12, sizeof s_dataAscii12);
   loadFile("res/gbfont12.bin", s_dataGB12, sizeof s_dataGB12);
   loadFile("res/ascii8.bin", s_dataAscii8, sizeof s_dataAscii8);
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
   m_memGraph = m_mem + addr;
}

void Device::setTextAddr(uint16_t addr) {
   m_memText = m_mem + addr;
}

void Device::setKeyAddr(uint16_t addr) {
   m_memKey = m_mem + addr;
}

void Device::setKeyMapAddr(uint16_t addr) {
   m_memKeyMap = m_mem + addr;
}

void Device::moveRow() {
   lock_guard<mutex> lock(m_mutGraph);
   memmove(m_memText, m_memText + 20, 80);
   memset(m_memText + 80, 0, 20);
   m_y--;
   memmove(m_memGraph, m_memGraph + 320, 1600 - 320);
   memset(m_memGraph + 1600 - 320, 0, 320);
}

void Device::appendText(const char *s, int size) {
   for (size_t i = 0; i < size; i++) {
      if (static_cast<uint8_t>(s[i]) > 160 && m_x == 19) {
         m_memText[m_y * 20 + 19] = 0;
         m_x = 0;
         m_y++;
      }
      if (m_y >= 5) {
         moveRow();
      }
      m_memText[m_y * 20 + m_x++] = static_cast<uint8_t>(s[i]);
      if (static_cast<uint8_t>(s[i]) > 160 && i < size - 1) {
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
   for (uint8_t *t = m_memText + m_y * 20 + i; i < 20; i++) {
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
      uint8_t *c = m_memText, *f, *g; //f字库 g屏幕缓冲
      int ff;

      for (int j = 0; j < 5; j++) {
         for (int i = 0; i < 20; i++) {
            g = j * 320 + i + m_memGraph;
            if (*c > 160) {
               ff = (*c << 8) | c[1];
               if (ff >= 0xf8a1 && ff <= 0xfdd9 && c[1] > 160) { //wqx图形
                  f = s_dataImage + (((*c - 248) * 94 + (c[1] - 161)) << 5);
               } else {
                  ff = *c - 161;
                  if (ff > 8)
                     ff -= 6;
                  ff = (ff * 94 + (c[1] - 161)) << 5;
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
               f = *c * 16 + s_dataAscii16;
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

   for (int i = 0; i <= m_x; ++i) {
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

inline void Device::py2GB(uint8_t *py, int size, const uint8_t *&list,
                   int &len) {
   if (0 == size) {
      len = 0;
      return;
   }
   py[size] = 0;

   for (int i = 0, l = sizeof s_dataPY - size; i < l; i += 11) {
      if (*py == s_dataPY[i]) {
         for (int j = 1; j <= size; ++j) {
            if (s_dataPY[i + j] != py[j])
               goto L1;
         }

         len = *reinterpret_cast<int16_t *>(s_dataPY + i + 7) << 1;
         int offset = *reinterpret_cast<int16_t *>(s_dataPY + i + 9) - 44;
         if (len < 0) {
            len = 0;
         } else {
            list = s_dataGBChar + offset;
         }
         return;
      }
L1:   ;
   }

   len = 0;
}

string Device::input() {
   static const uint8_t INPUT_FRAME[] = {
         255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 240,
         128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 127, 240,
         128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 120, 112,
         128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 115, 48,
         128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 103, 240,
         128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 103, 240,
         128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 102, 48,
         128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 115, 48,
         128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 120, 48,
         128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 127, 240,
         128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 127, 240,
         255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 240,
         128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16,
         128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16,
         128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16,
         128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16,
         128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16,
         128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16,
         128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16,
         128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16,
         128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16,
         128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16,
         128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16,
         128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16,
         128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16,
         255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 240,
   };
   static const uint8_t OPT_NUM[][6] = {
      { 64, 192, 64, 64, 64, 224 },
      { 64, 160, 32, 64, 128, 224 },
      { 192, 32, 64, 32, 32, 192 },
      { 32, 96, 160, 224, 32, 32 },
      { 224, 128, 192, 32, 32, 192 }
   };
   static const uint8_t OPT_PGUP[] = { 0x20, 0x70, 0xf8 };
   static const uint8_t OPT_PGDN[] = { 0xf8, 0x70, 0x20 };

   constexpr int MAX_STR = 255;

   m_enableCursor = true;
   m_gui->beginInput();
   m_gui->switchIM(IGui::InputMethod::EN);

   uint8_t graphBackup[1600];
   uint8_t textBackup[100];
   memcpy(graphBackup, m_memGraph, 1600); // 备份原graph，不需要lock
   memcpy(textBackup, m_memText, 100); // 备份原text
   uint8_t x0 = m_x, y0 = m_y; // 原坐标
   string resultStr;
   uint8_t pinyin[10];
   int pySize;
   IGui::InputMethod inputMethod = IGui::InputMethod::EN;
   const uint8_t *gbList; int gbLen, gbCurPage; // 候选字列表

   while (true) {
      // 恢复原界面
      {
         lock_guard<mutex> lock(m_mutGraph);
         memcpy(m_memGraph, graphBackup, 1600);
      }
      memcpy(m_memText, textBackup, 100);
      m_x = x0; m_y = y0;
      appendText(resultStr);
      updateLCD();

      if (IGui::InputMethod::CH == inputMethod) { // 中文
         bool canDown = false, canUp = false;
         int gbTotal = 0; // 当前页可选择的字数1~5

         if (pySize > 0) {
            // 输入框
            int x = m_x * 8, y = m_y;
            if (y < 2)
               y = y + 1 << 4;
            else
               y = y * 16 - 28;
            if (x > 160 - 102)
               x = 160 - 102;
            rectangle(x, y, x + 101, y + 27, false, DrawMode::CLEAR);
            paint(INPUT_FRAME, x + 1, y + 1, 100, 26, PaintMode::COPY);
            tinyTextOut(pinyin, pySize, x + 3, y + 3);
            gbTotal = gbLen / 2 - gbCurPage * 5;
            if (canDown = gbTotal > 5)
               gbTotal = 5;
            for (int i = 0; i < gbTotal; ++i) {
               paint(OPT_NUM[i], x + 3 + i * 18, y + 14, 3, 6, PaintMode::COPY);
               drawChar12(gbList + gbCurPage * 10 + i * 2,
                          x + 7 + i * 18, y + 14);
            }
            if (canUp = gbCurPage > 0)
               paint(OPT_PGUP, x + 93, y + 15, 5, 3, PaintMode::COPY);
            if (canDown)
               paint(OPT_PGDN, x + 93, y + 21, 5, 3, PaintMode::COPY);
         }

         // 按键
         switch (uint8_t key = getKey()) {
         case ' ': m_tmpchar = '1';
            // fall through
         case 'b': case 'n': case 'm': case 'g': case 'h': // 选择候选字
            if (m_tmpchar > '0' && m_tmpchar <= '0' + gbTotal
                && resultStr.size() < MAX_STR - 1) {
               resultStr.append(reinterpret_cast<const char *>(
                                gbList + gbCurPage * 10 + (m_tmpchar - '1') * 2),
                                2);
               pySize = 0;
               break;
            }
            // fall through
         default: // 输入拼音
            if (key >= 'a' && key <= 'z' && pySize < 6) {
               pinyin[pySize] = key;
               gbCurPage = 0;
               py2GB(pinyin, ++pySize, gbList, gbLen);
            }
            break;
         case 29: // 删除
            if (pySize > 0) { // 有拼音
               gbCurPage = 0;
               py2GB(pinyin, --pySize, gbList, gbLen);
            } else if (resultStr.size()) { // 没有拼音，删除已输入的字符
               if (resultStr.back() & 128) {
                  // 最后一个字符是全角字符
                  resultStr.pop_back();
                  if (resultStr.size())
                     resultStr.pop_back();
               } else {
                  resultStr.pop_back();
               }
            }
            break;
         case 23: case 19: case 20: // 上翻页
            if (canUp)
               --gbCurPage;
            break;
         case 14: case 22: case 21: // 下翻页
            if (canDown)
               ++gbCurPage;
            break;
         case 26: // Shift切换中英文
            inputMethod = IGui::InputMethod::EN;
            m_gui->switchIM(inputMethod);
            break;
         case 13:
            if (0 == pySize) {
               goto Lreturn;
            }
            resultStr.append(reinterpret_cast<const char *>(pinyin),
                             min<size_t>(static_cast<size_t>(pySize),
                                         MAX_STR - resultStr.size()));
            pySize = 0;
            break;
         }
      } else { // 英文
         switch (getKey()) {
         case 26:
            inputMethod = IGui::InputMethod::CH;
            m_gui->switchIM(inputMethod);
            pySize = 0;
            gbLen = 0;
            gbCurPage = 0;
            break;
         case 13:
            goto Lreturn;
         case 29:
            if (resultStr.size()) {
               if (resultStr.back() & 128) {
                  // 最后一个字符是全角字符
                  resultStr.pop_back();
                  if (resultStr.size())
                     resultStr.pop_back();
               } else {
                  resultStr.pop_back();
               }
            }
            break;
         default:
            if (m_tmpchar > 31 && !(m_tmpchar & 128)) {
               resultStr += m_tmpchar;
            }
            break;
         }
      }
   }

Lreturn:
   m_enableCursor = false;
   m_gui->endInput();
   // 恢复原界面
   {
      lock_guard<mutex> lock(m_mutGraph);
      memcpy(m_memGraph, graphBackup, 1600);
   }
   memcpy(m_memText, textBackup, 100);
   m_x = x0; m_y = y0;
   appendText(resultStr);
   nextRow();
   updateLCD();

   return resultStr;
}

uint8_t Device::getKey() {
   m_enableCursor = true;
   int a;
   do {
      {
         lock_guard<mutex> lock(m_mutKey);
         a = static_cast<int>(*m_memKey) - 128;
      }
      if (m_gui->isStopped())
         throw GVB::Quit();
      this_thread::sleep_for(chrono::milliseconds(50));
   } while (a < 0);
   m_enableCursor = false;
   m_gui->m_displayCursor = false;
   {
      lock_guard<mutex> lock(m_mutKey);
      *m_memKey &= 127;
   }
   return static_cast<uint8_t>(a);
}

void Device::onKeyDown(int key, char c) {
   lock_guard<mutex> lock(m_mutKey);
   *m_memKey = static_cast<uint8_t>(128 + key);
   if (int m = m_keyMap[key]) {
      m_memKeyMap[m >> 8] &= ~(m & 255);
   }
   m_tmpchar = c;
}

void Device::onKeyUp(int key) {
   if (int m = m_keyMap[key]) {
      m_memKeyMap[m >> 8] |= m & 255;
   }
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
   uint8_t *p = m_mem + addr;
   if (m_memKey == p) {
      lock_guard<mutex> lock(m_mutKey);
      return *p;
   }
   if (p >= m_memGraph && p < m_memGraph + 1600) {
      lock_guard<mutex> lock(m_mutGraph);
      return *p;
   }
   return *p;
}

void Device::poke(uint16_t addr, uint8_t value) {
   uint8_t *p = m_mem + addr;
   if (m_memKey == p) {
      lock_guard<mutex> lock(m_mutKey);
      *p = value;
      if (value == 155)
         throw GVB::Quit();
   } else if (p >= m_memGraph && p < m_memGraph + 1600) {
      {
         lock_guard<mutex> lock(m_mutGraph);
         *p = value;
      }
      addr -= m_memGraph - m_mem;
      int x = addr % 20;
      int y = addr / 20;
      m_gui->update(x << 3, y, x + 1 << 3, y);
   } else if (p < m_memKeyMap || p >= m_memKeyMap + 8) {
      *p = value;
   }
}

void Device::sleep(int ticks) {
   m_gui->sleep(ticks);
}

// 8x8小字
void Device::tinyTextOut(const uint8_t *str, int size, int x, int y) {
   while (--size >= 0) {
      paint(s_dataAscii8 + *str++ * 8, x, y, 8, 8, PaintMode::COPY);
      x += 8;
   }
}

// 12x12小字
void Device::drawChar12(const uint8_t *str, int x, int y) {
   if (*str > 160) {
      int ff = *str - 161;
      if (ff > 8)
         ff -= 6;
      ff = (ff * 94 + (str[1] - 161)) * 24;
      if (ff < 0 || ff + 24 > sizeof s_dataGB12) {
         ff = 85 * 24; //黑方块
      }
      paint(s_dataGB12 + ff, x, y, 12, 12, PaintMode::COPY);
   } else {
      paint(s_dataAscii12 + *str * 12, x, y, 6, 12, PaintMode::COPY);
   }
}

void Device::paint(const uint8_t *ptr, int x, int y, uint8_t w, uint8_t h,
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
         ptr += (-x) / 8;
         unuseDataBits = (-x) % 8;
         w += x;
         x = 0;
      }
      if (y < 0) {
         ptr += -bw * y;
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
            mapData[i] = ptr[i];
         }
         ptr += bw;
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