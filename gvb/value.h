#ifndef GVBASIC_VALUE_H
#define GVBASIC_VALUE_H

namespace gvbsim {

struct Value {
   enum class Type {
      REAL = 1, STRING,
      RVAL_MASK = 0x1f,
      // int经过rval mask后变成real，用于表达式类型检查(表达式中只有real和string类型)
      INT = 0x21
   };

   static const char *toString(Type type);
};

}

#endif //GVBASIC_VALUE_H
