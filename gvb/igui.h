#ifndef GVBASIC_IGUI_H
#define GVBASIC_IGUI_H

namespace gvbsim {


class IGui {

public:
   bool m_displayCursor; // should be atomic　??

public:
   virtual ~IGui() { }

public:
   virtual void update() = 0;
   // 左上角 (x1, y1) - (x2, y2) 右下角(包括)
   virtual void update(int x1, int y1, int x2, int y2) = 0;

   virtual void sleep(int ticks) = 0;

   void flipCursor() { m_displayCursor = !m_displayCursor; }
};

}

#endif //GVBASIC_IGUI_H
