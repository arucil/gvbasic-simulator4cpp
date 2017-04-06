#ifndef GVBASIC_FUNC_H
#define GVBASIC_FUNC_H

#include "value.h"

namespace gvbsim {

struct Func {
   enum class Type {
      ABS = 1, ASC, ATN, CHR, COS, CVI, MKI, CVS, MKS, EXP, INT, LEFT, LEN, LOG,
      MID, POS, RIGHT, RND, SGN, SIN, SQR, STR, TAN, VAL, PEEK, FEOF, LOF,
      NEG, NOT,
      TAB = 0x81, SPC
   };

   enum { PRINT_FUNC_MASK = 0x80 };

public:
   static Value::Type getReturnType(Type);
   static Value::Type getParamType(Type, int index);
};

}

#endif //GVBASIC_FUNC_H
