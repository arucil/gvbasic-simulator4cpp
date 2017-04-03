#include "func.h"

using namespace gvbsim;

Value::Type Func::getReturnType(Type f) {
   switch (f) {
   case Type::CHR:
   case Type::STR:
   case Type::MKI:
   case Type::MKS:
   case Type::LEFT:
   case Type::MID:
   case Type::RIGHT:
      return Value::Type::STRING;
   default:
      return Value::Type::REAL;
   }
}

Value::Type Func::getParamType(Type f, int index) {
   switch (f) {
   case Type::ASC:
   case Type::LEN:
   case Type::CVI:
   case Type::CVS:
   case Type::VAL:
      return Value::Type::STRING;
   case Type::LEFT:
   case Type::RIGHT:
   case Type::MID:
      if (0 == index)
         return Value::Type::STRING;
   default:
      return Value::Type::REAL;
   }
}
