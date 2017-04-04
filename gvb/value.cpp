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


void Value::destroy(Value *v) {
   switch (v->type) {
   case Value::VarType::ID:
      delete static_cast<Single *>(v); // 确保子类的析构函数能够调用
      break;
   case Value::VarType::ARRAY:
      delete static_cast<Array *>(v);
      break;
   default:
      assert(0);
   }
}