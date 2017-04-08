#ifndef GVBASIC_IGUI_H
#define GVBASIC_IGUI_H

namespace gvbsim {


class IGui {

public:
   virtual ~IGui() { }

public:
   virtual void update() = 0;
   virtual void update(int x1, int y1, int x2, int y2) = 0;
};

}

#endif //GVBASIC_IGUI_H
