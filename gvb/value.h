#ifndef GVBASIC_VALUE_H
#define GVBASIC_VALUE_H

namespace gvbsim {

struct Value {
   enum class Type {
      STRING, REAL, INT
   };

   static const char *toString(Type type);
};

}

#endif //GVBASIC_VALUE_H
