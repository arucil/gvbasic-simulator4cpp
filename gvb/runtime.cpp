#include "gvb.h"
#include <ctime>
#include <cassert>
#include <cmath>
#include <cstdint>
#include "device.h"
#include "compile.h"
#include "tree.h"
#include "real.h"

using namespace std;
using namespace gvbsim;


#define rerror(...)   error(m_line, m_label, __VA_ARGS__)


void GVB::execute() {
   if (!isBuilt())
      return;

   m_rand.setSeed(static_cast<uint32_t>(time(nullptr)));
   m_dataMan.restore();
   clearEnv();
   clearStack();
   m_funcs.clear();

   m_device->setMode(Device::ScreenMode::TEXT);

   traverse(m_head);
}

void GVB::clearEnv() {
   for (auto &i : m_env) {
      Value::destroy(i.second);
   }
}

void GVB::clearStack() {
   m_stack.clear();
   m_subs.clear();
   m_fors.clear();
   m_whiles.clear();
}

void GVB::traverse(Stmt *s) {
   while (s) {
      switch (s->type) {
      case Stmt::Type::NEWLINE:
         m_line = static_cast<NewLine *>(s)->line;
         m_label = static_cast<NewLine *>(s)->label;
         break;

      case Stmt::Type::CLEAR:
         clearStack();
         clearEnv();
         m_funcs.clear();
         m_dataMan.restore();
         break;

      case Stmt::Type::END:
         s = nullptr;
         continue;

      case Stmt::Type::NONE:
      case Stmt::Type::BEEP:
      case Stmt::Type::PLAY:
      case Stmt::Type::INVERSE:
         break;

      case Stmt::Type::INKEY:
         m_device->getKey();
         break;

      case Stmt::Type::CLS:
         m_device->cls();
         break;

      case Stmt::Type::GRAPH:
         m_device->setMode(Device::ScreenMode::GRAPH);
         break;

      case Stmt::Type::TEXT:
         m_device->setMode(Device::ScreenMode::TEXT);
         break;

      case Stmt::Type::DIM:
         exe_dim(static_cast<Dim *>(s));
         break;

      case Stmt::Type::ASSIGN:
         exe_assign(static_cast<Assign *>(s));
         break;
      }
   }
}

void GVB::exe_dim(Dim *d1) {
   Id *id1 = d1->id;

   switch (id1->type) {
   case Expr::Type::ID: {
      if (m_env.count(id1->id)) {
         rerror("Redefined variable: %s", id1->id);
      }

      switch (id1->vtype) {
      case Value::Type::INT:
         m_env[id1->id] = Value::make<Single>(0);
         break;
      case Value::Type::REAL:
         m_env[id1->id] = Value::make<Single>(0.0);
         break;
      case Value::Type::STRING:
         m_env[id1->id] = Value::make<Single>("");
         break;
      default:
         assert(0);
      }
      break;
   }

   case Expr::Type::ARRAYACCESS: {
      ArrayAccess *ac1 = static_cast<ArrayAccess *>(id1);
      if (m_env.count(id1->id)) {
         rerror("Redefined array: %s", id1->id);
      }

      Array *a1 = Value::make<Array>();
      unsigned total = 1;
      for (auto i : ac1->indices) {
         evalPop(i);
         if (m_top.rval < 0 || m_top.rval > UINT16_MAX) {
            rerror("Bad index in DIM array: %s, [%f]", m_top.rval);
         }
         unsigned t = static_cast<unsigned>(m_top.rval) + 1;
         a1->bounds.push_back(t);
         total *= t;
      }

      switch (a1->vtype = id1->vtype) {
      case Value::Type::INT:
         a1->nums.resize(total, Array::Internal(0));
         break;
      case Value::Type::REAL:
         a1->nums.resize(total, Array::Internal(0.0));
         break;
      case Value::Type::STRING:
         a1->strs.resize(total, "");
         break;
      default:
         assert(0);
      }
      m_env[id1->id] = a1;
   }
   default:
      assert(0);
   }
}

void GVB::exe_assign(Assign *a1) {

}

inline void GVB::evalPop(Expr *e1) {
   eval(e1);
   m_top = m_stack.back();
   m_stack.pop_back();
}

void GVB::eval(Expr *e1) {
   switch (e1->type) {
   case Expr::Type::ID:
      return eval_id(static_cast<Id *>(e1));

   case Expr::Type::ARRAYACCESS:
      return eval_access(static_cast<ArrayAccess *>(e1));

   case Expr::Type::REAL:
      // 编译时确保rval一定有效，不需要再检查
      m_stack.push_back(Single(static_cast<Real *>(e1)->rval));
      break;

   case Expr::Type::STRING:
      m_stack.push_back(Single(static_cast<Str *>(e1)->sval));
      break;

   case Expr::Type::INKEY:
      m_stack.push_back(Single(string(1u, static_cast<char>(m_device->getKey()))));
      break;

   case Expr::Type::FUNCCALL:
      return eval_func(static_cast<FuncCall *>(e1));

   case Expr::Type::BINARY:
      return eval_binary(static_cast<Binary *>(e1));

   case Expr::Type::USERCALL:
      return eval_usercall(static_cast<UserCall *>(e1));

   default:
      assert(0);
   }
}

inline void GVB::eval_id(Id *id1) {
   auto &i1 = m_env[id1->id];
   if (nullptr == i1) { // 变量不存在
      switch (Compiler::getIdType(id1->id)) {
      case Value::Type::INT:
         i1 = Value::make<Single>(0);
         break;
      case Value::Type::REAL:
         i1 = Value::make<Single>(0.0);
         break;
      case Value::Type::STRING:
         i1 = Value::make<Single>("");
         break;
      default:
         assert(0);
      }
   }

   if (Value::Type::INT == i1->vtype)
      m_stack.push_back(Single(static_cast<double>(static_cast<Single *>(i1)->ival)));
   else
      m_stack.push_back(*static_cast<Single *>(i1));
}

inline void GVB::eval_access(ArrayAccess *ac1) {
   auto &v1 = m_env[ac1->id];
   auto a1 = static_cast<Array *>(v1);

   if (nullptr == a1) { // 数组不存在
      v1 = a1 = Value::make<Array>();
      a1->bounds.resize(ac1->indices.size(), 11u);
      unsigned total = 1;
      for (auto j = ac1->indices.size(); j > 0; --j)
         total *= 11;

      switch (a1->vtype = Compiler::getIdType(ac1->id)) {
      case Value::Type::INT:
         a1->nums.resize(total, Array::Internal(0));
         break;
      case Value::Type::REAL:
         a1->nums.resize(total, Array::Internal(0.0));
         break;
      case Value::Type::STRING:
         a1->strs.resize(total, "");
         break;
      default:
         assert(0);
      }
   }

   if (ac1->indices.size() != a1->bounds.size()) {
      rerror("Array dimension mismatch: expecting %i, got %i",
             static_cast<int>(a1->bounds.size()),
             static_cast<int>(ac1->indices.size()));
   }

   evalPop(ac1->indices[0]);
   if (m_top.rval < 0 || m_top.rval >= a1->bounds[0]) {
      rerror("Bad index in array: %s 0[%f]", ac1->id, m_top.rval);
   }

   unsigned total = static_cast<unsigned>(m_top.rval);
   for (size_t i = 0; i < ac1->indices.size(); ++i) {
      evalPop(ac1->indices[i]);
      if (m_top.rval < 0 || m_top.rval >= a1->bounds[i]) {
         rerror("Bad index in array: %s %i[%f]", ac1->id,
                static_cast<int>(i), m_top.rval);
      }
      total = total * a1->bounds[i] + static_cast<unsigned>(m_top.rval);
   }

   switch (a1->vtype) {
   case Value::Type::INT:
      m_stack.push_back(Single(a1->nums[total].ival));
      break;
   case Value::Type::REAL:
      m_stack.push_back(Single(a1->nums[total].rval));
      break;
   case Value::Type::STRING:
      m_stack.push_back(Single(a1->strs[total]));
      break;
   default:
      assert(0);
   }
}

inline void GVB::eval_binary(Binary *b1) {
   eval(b1->left);
   evalPop(b1->right);

   switch (b1->op) {
   case '+':
      if (Value::Type::REAL == m_top.vtype) {
         auto res = RealHelper::validate(m_stack.back().rval += m_top.rval);
         assert(res != RealHelper::Result::IS_NAN); // 加法会nan ?
         if (RealHelper::Result::IS_INF == res) {
            rerror("Number overflow");
         }
      } else
         m_stack.back().sval += m_top.sval;
      break;

   case '-': {
      auto res = RealHelper::validate(m_stack.back().rval -= m_top.rval);
      assert(res != RealHelper::Result::IS_NAN);
      if (RealHelper::Result::IS_INF == res) {
         rerror("Number overflow");
      }
      break;
   }

   case '*': {
      auto res = RealHelper::validate(m_stack.back().rval *= m_top.rval);
      assert(res != RealHelper::Result::IS_NAN);
      if (RealHelper::Result::IS_INF == res) {
         rerror("Number overflow");
      }
      break;
   }

   case '/': {
      if (0.0 == m_top.rval) {
         rerror("Division by zero");
      }
      auto res = RealHelper::validate(m_stack.back().rval /= m_top.rval);
      // 0 / 0  => nan
      // x / 0  => inf
      if (res != RealHelper::Result::IS_VALID) {
         rerror("Number overflow");
      }
      break;
   }

   case '^': {
      errno = 0;
      double r = pow(m_stack.back().rval, m_top.rval);
      int err = errno;
      auto res = RealHelper::validate(m_stack.back().rval = r);
      if (ERANGE == err || RealHelper::Result::IS_INF == res) {
         rerror("Number overflow");
      }
      if (err || RealHelper::Result::IS_VALID != res) {
         rerror("Illegal operand");
      }
      break;
   }

#define CMP(op)    \
   if (Value::Type::REAL == m_top.vtype) { \
      m_stack.back().rval = m_stack.back().rval op m_top.rval; \
   } else { \
      m_stack.back().vtype = Value::Type::REAL; \
      m_stack.back().rval = m_stack.back().sval op m_top.sval; \
   } \
   break

   case '=': CMP(==);
   case Token::NEQ: CMP(!=);
   case Token::GE: CMP(>=);
   case Token::LE: CMP(<=);
   case '>': CMP(>);
   case '<': CMP(<);

#undef CMP

   case Token::AND:
      m_stack.back().rval = m_stack.back().rval != 0. && m_top.rval != 0.;
      break;

   case Token::OR:
      m_stack.back().rval = m_stack.back().rval != 0. || m_top.rval != 0.;
      break;
   }
}

inline void GVB::eval_func(FuncCall *fc) {
   eval(fc->expr1);

   switch (fc->ftype) {
   case Func::Type::NEG:
      m_stack.back().rval = -m_stack.back().rval;
      break;

   case Func::Type::NOT:
      m_stack.back().rval = 0. == m_stack.back().rval;
      break;

   case Func::Type::ABS:
      m_stack.back().rval = abs(m_stack.back().rval);
      break;

   case Func::Type::ASC:
      if (m_stack.back().sval.empty()) {
         rerror("Illegal argument is ASC: empty string");
      }
      m_stack.back().vtype = Value::Type::REAL;
      m_stack.back().rval = m_stack.back().sval[0] & 0xff;
      break;

   case Func::Type::ATN: {
      auto res = RealHelper::validate(m_stack.back().rval =
                                            atan(m_stack.back().rval));
      assert(res == RealHelper::Result::IS_VALID);
      break;
   }

   case Func::Type::CHR: {
      if (m_stack.back().rval < 0 || m_stack.back().rval > 255) {
         rerror("Illegal argument: CHR$(%f)", m_stack.back().rval);
      }

      char c = static_cast<char>(static_cast<unsigned>(m_stack.back().rval));
      m_stack.back().vtype = Value::Type::STRING;
      m_stack.back().sval.assign(&c, &c + 1);
      break;
   }

   case Func::Type::COS: {
      auto res = RealHelper::validate(m_stack.back().rval =
                                            cos(m_stack.back().rval));
      assert(res == RealHelper::Result::IS_VALID);
      break;
   }

   case Func::Type::SIN: {
      auto res = RealHelper::validate(m_stack.back().rval =
                                            sin(m_stack.back().rval));
      assert(res == RealHelper::Result::IS_VALID);
      break;
   }

   case Func::Type::CVI: // 2-byte => int
      if (m_stack.back().sval.size() < 2) {
         rerror("String too short in CVI: LEN(%s)=%i",
                m_stack.back().sval,
                static_cast<int>(m_stack.back().sval.size()));
      }
      m_stack.back().vtype = Value::Type::REAL;
      m_stack.back().rval = *reinterpret_cast<const int16_t *>(
            m_stack.back().sval.data());
      break;

   case Func::Type::MKI: { // int => 2-byte
      if (m_stack.back().rval < INT16_MIN || m_stack.back().rval > INT16_MAX) {
         rerror("Illegal argument in MKI: %f",
                m_stack.back().rval);
      }
      int16_t s1 = static_cast<int16_t>(m_stack.back().rval);
      m_stack.back().vtype = Value::Type::STRING;
      m_stack.back().sval.assign(reinterpret_cast<char *>(&s1),
                                 reinterpret_cast<char *>(&s1) + 2);
      break;
   }

   case Func::Type::CVS: // 5-byte => double
      if (m_stack.back().sval.size() < 5) {
         rerror("String too short in CVS: LEN(%s)=%i",
                m_stack.back().sval,
                static_cast<int>(m_stack.back().sval.size()));
      }
      m_stack.back().vtype = Value::Type::REAL;
      m_stack.back().rval = RealHelper::toDouble(
            *reinterpret_cast<const uint64_t *>(m_stack.back().sval.data()));
      break;

   case Func::Type::MKS: { // double => 5-byte
      uint64_t d = RealHelper::fromDouble(m_stack.back().rval);
      m_stack.back().vtype = Value::Type::STRING;
      m_stack.back().sval.assign(reinterpret_cast<char *>(&d),
                                 reinterpret_cast<char *>(&d) + 5);
      break;
   }

   case Func::Type::EXP: {
      auto res = RealHelper::validate(m_stack.back().rval =
                                            exp(m_stack.back().rval));
      assert(res != RealHelper::Result::IS_NAN);
      if (RealHelper::Result::IS_NAN == res) {
         rerror("Number overflow");
      }
      break;
   }

   case Func::Type::INT:
      m_stack.back().rval = floor(m_stack.back().rval); // 需要validate ?
      break;

   case Func::Type::LEN:
      m_stack.back().vtype = Value::Type::REAL;
      m_stack.back().rval = m_stack.back().sval.size();
      break;

   case Func::Type::LEFT:
      evalPop(fc->expr2);
      if (m_top.rval < 0) {
         rerror("Illegal count in LEFT: %f", m_top.rval);
      }
      if (m_stack.back().sval.size() > m_top.rval) {
         m_stack.back().sval.resize(static_cast<unsigned>(m_top.rval));
      }
      break;

   case Func::Type::RIGHT: {
      evalPop(fc->expr2);
      if (m_top.rval < 0) {
         rerror("Illegal count in RIGHT: %f", m_top.rval);
      }
      unsigned size = static_cast<unsigned>(m_stack.back().sval.size());
      if (size > m_top.rval) {
         m_stack.back().sval.erase(0, size - static_cast<unsigned>(m_top.rval));
      }
      break;
   }

   case Func::Type::LOG: {
      double &r = m_stack.back().rval;
      errno = 0;
      r = log(r);
      int err = errno;
      auto res = RealHelper::validate(r);
      if (ERANGE == err || RealHelper::Result::IS_INF == res) {
         rerror("Number overflow");
      }
      if (err || RealHelper::Result::IS_VALID != res) {
         rerror("Illegal operand");
      }
      break;
   }

   case Func::Type::MID: {
      evalPop(fc->expr2);
      auto &s = m_stack.back().sval;
      if (m_top.rval < 1 || m_top.rval > s.size()) {
         rerror("Illegal offset in MID: %f, LEN(%s)=%i",
                m_top.rval, m_stack.back().sval,
                static_cast<int>(s.size()));
      }
      unsigned offset = static_cast<unsigned>(m_top.rval);

      if (nullptr == fc->expr3)
         m_top.rval = 1.;
      else {
         evalPop(fc->expr3);
      }
      if (m_top.rval < 1.) {
         rerror("Illegal count in MID: %f", m_top.rval);
      }
      --offset;
      unsigned count = static_cast<unsigned>(m_top.rval);
      if (offset + m_top.rval > s.size())
         count = static_cast<unsigned>(s.size() - offset);
      s = s.substr(offset, count);
      break;
   }

   case Func::Type::POS:
      m_stack.back().rval = m_device->getX() + 1;
      break;

   case Func::Type::RND:
      if (0. == m_stack.back().rval)
         m_stack.back().rval = m_rand.stationary();
      else if (m_stack.back().rval > 0.)
         m_stack.back().rval = m_rand.random();
      else
         m_stack.back().rval = m_rand.sequence();
      break;

   case Func::Type::SGN: {
      auto &r = m_stack.back().rval;
      if (r < 0.0)
         r = -1.0;
      else if (r > 0.0)
         r = 1.0;
      break;
   }
   }
}

inline void GVB::eval_usercall(UserCall *uc) {

}