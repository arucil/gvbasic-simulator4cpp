#ifndef GVBASIC_IGUI_H
#define GVBASIC_IGUI_H

#include <atomic>

namespace gvbsim {


class IGui {

public:
   std::atomic_bool m_displayCursor;

public:
   virtual ~IGui() { }

public:
   virtual void update() = 0;
   // 左上角 (x1, y1) - (x2, y2) 右下角(包括)
   virtual void update(int x1, int y1, int x2, int y2) = 0;

   virtual void sleep(int ticks) = 0;
};

}

#endif //GVBASIC_IGUI_H
