#ifndef GVBASIC_VALUE_H
#define GVBASIC_VALUE_H

#include <string>
#include <vector>
#include <type_traits>

namespace gvbsim {


struct Value {
   enum class Type {
      REAL = 1, STRING,
      // int经过rval mask后变成real，用于表达式类型检查(表达式中只有real和string类型)
      INT = 0x21
   };

public:
   enum { RVAL_MASK = 0x1f };

   static const char *toString(Type type);
};

// 单个值
struct Single {
   Value::Type vtype;
   union {
      int ival;
      double rval;
   };
   std::string sval;

public:
   Single() { }
   Single(int ival) : vtype(Value::Type::INT), ival(ival) { }
   Single(double rval) : vtype(Value::Type::REAL), rval(rval) { }
   Single(const std::string &s) : vtype(Value::Type::STRING), sval(s) { }
   Single(const char *s) : vtype(Value::Type::STRING), sval(s) { }
};

// 数组
struct Array {
   Value::Type vtype;
   union Number {
      int ival;
      double rval;

      Number(int i) : ival(i) { }
      Number(double r) : rval(r) { }
   };

public:
   std::vector<unsigned> bounds;
   std::vector<Number> nums;
   std::vector<std::string> strs;
};

}

#endif //GVBASIC_VALUE_H
