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

   enum class VarType {
      ID, ARRAY
   };

public:
   enum { RVAL_MASK = 0x1f };

   static const char *toString(Type type);

public:
   VarType type;
   Type vtype;

protected:
   Value(VarType t) : type(t) { }
   Value(VarType t, Type vt) : type(t), vtype(vt) { }

public:
   template <typename T, typename... Ts>
   static T *make(Ts...);

   static void destroy(Value *);
};

template <typename T, typename... Ts>
T *Value::make(Ts... args) {
   static_assert(std::is_base_of<Value, T>::value, "");

   return new T(args...);
}


// 单个值
struct Single : Value {
   union {
      int ival;
      double rval;
   };
   std::string sval;

public:
   Single() : Value(VarType::ID) { }
   Single(int ival) : Value(VarType::ID, Type::INT), ival(ival) { }
   Single(double rval) : Value(VarType::ID, Type::REAL), rval(rval) { }
   Single(const std::string &s) : Value(VarType::ID, Type::STRING), sval(s) { }
};

// 数组
struct Array : Value {
   union Internal {
      int ival;
      double rval;

      Internal(int i) : ival(i) { }
      Internal(double r) : rval(r) { }
   };

public:
   std::vector<unsigned> bounds;
   std::vector<Internal> nums;
   std::vector<std::string> strs;

public:
   Array() : Value(VarType::ARRAY) { }
};

}

#endif //GVBASIC_VALUE_H
