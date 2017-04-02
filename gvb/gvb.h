#ifndef GVBASIC_GVB_H
#define GVBASIC_GVB_H

#include "node_man.h"

namespace gvbsim {

class GVB {

public:
   static void error(int line, int label, const char *s, ...);
};

}

#endif //GVBASIC_GVB_H
