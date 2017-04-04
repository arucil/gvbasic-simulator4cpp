#include "value.h"
#include <cassert>

using namespace gvbsim;

const char *Value::toString(Type type) {
   switch (type) {
   case Type::STRING:
      return "string";
   case Type::REAL:
      return "real";
   case Type::INT:
      return "int";
   default:
      assert(0);
   }
}
