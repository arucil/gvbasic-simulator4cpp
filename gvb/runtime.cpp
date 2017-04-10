#include "gvb.h"
#include <cassert>
#include <cmath>
#include <stdexcept>
#include <cstdlib>
#include <algorithm>
#include "compile.h"
#include "tree.h"
#include "real.h"

using namespace std;
using namespace gvbsim;


#define rerror(...)   error(m_line, m_label, __VA_ARGS__)


void GVB::execute(uint32_t seed) {
   if (!reset(seed))
      return;

   traverse();
}

bool GVB::reset(uint32_t seed) {
   if (!isBuilt())
      return false;

   m_rand.setSeed(seed);
   m_dataMan.restore();
   clearEnv();
   clearStack();
   clearFiles();
   m_funcs.clear();

   m_device.setMode(Device::ScreenMode::TEXT);
   m_device.locate(0, 0);

   m_current = m_head;

   return true;
}


void GVB::clearEnv() {
   m_envVar.clear();
   m_envArray.clear();
}

void GVB::clearStack() {
   m_stack.clear();
   m_subs.clear();
   m_loops.clear();
}

void GVB::clearFiles() {
   m_files[0].close();
   m_files[1].close();
   m_files[2].close();
}

void GVB::traverse() {
   while (step()) {
   }
}

bool GVB::step() {
   if (nullptr == m_current)
      return false;

   Stmt *s = m_current;
   do {
      switch (s->type) {
      case Stmt::Type::NEWLINE:
         m_line = static_cast<NewLine *>(s)->line;
         m_label = static_cast<NewLine *>(s)->label;
         break;

      case Stmt::Type::CLEAR:
         clearStack();
         clearEnv();
         clearFiles();
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
         m_device.getKey();
         break;

      case Stmt::Type::CLS:
         m_device.cls();
         break;

      case Stmt::Type::GRAPH:
         m_device.setMode(Device::ScreenMode::GRAPH);
         break;

      case Stmt::Type::TEXT:
         m_device.setMode(Device::ScreenMode::TEXT);
         break;

      case Stmt::Type::DIM:
         exe_dim(static_cast<Dim *>(s));
         break;

      case Stmt::Type::ASSIGN:
         exe_assign(static_cast<Assign *>(s));
         break;

      case Stmt::Type::FOR:
         exe_for(static_cast<For *>(s));
         break;

      case Stmt::Type::NEXT:
         s = exe_next(static_cast<Next *>(s));
         break;

      case Stmt::Type::WHILE:
         s = exe_while(static_cast<While *>(s));
         continue; // 注意是continue

      case Stmt::Type::WEND:
         s = exe_wend();
         continue; // 注意是continue

      case Stmt::Type::PRINT:
         exe_print(static_cast<Print *>(s));
         break;

      case Stmt::Type::IF:
         s = exe_if(static_cast<If *>(s));
         continue; // 注意是continue

      case Stmt::Type::GOSUB:
         if (m_subs.size() >= UINT16_MAX) {
            rerror("Stack overflow in GOSUB");
         }
         m_subs.push_back(Sub(m_line, m_label, s->next));
         // fall through

      case Stmt::Type::GOTO:
         s = static_cast<Goto *>(s)->stm;
         continue; //

      case Stmt::Type::RETURN:
         if (m_subs.empty()) {
            rerror("Return without gosub");
         }
         m_line = m_subs.back().line;
         m_label = m_subs.back().label;
         s = m_subs.back().stmt;
         m_subs.pop_back();
         continue; //

      case Stmt::Type::POP:
         if (m_subs.empty()) {
            rerror("Pop without gosub");
         }
         m_subs.pop_back();
         break;

      case Stmt::Type::ON:
         s = exe_on(static_cast<On *>(s));
         continue; //

      case Stmt::Type::DEFFN:
         // 如果自定义函数已存在，则覆盖
         m_funcs[static_cast<DefFn *>(s)->fnName] = static_cast<DefFn *>(s);
         break;

      case Stmt::Type::RESTORE:
         if (static_cast<Restore *>(s)->label == Restore::NO_LABEL) {
            m_dataMan.restore();
         } else {
            m_dataMan.restore(static_cast<Restore *>(s)->label);
         }
         break;

      case Stmt::Type::LOCATE:
         exe_locate(static_cast<Locate *>(s));
         break;

      case Stmt::Type::POKE:
         exe_poke(static_cast<Poke *>(s));
         break;

      case Stmt::Type::CALL:
         exe_call(static_cast<Call *>(s));
         break;

      case Stmt::Type::SWAP:
         exe_swap(static_cast<Swap *>(s));
         break;

      case Stmt::Type::READ:
         exe_read(static_cast<Read *>(s));
         break;

      case Stmt::Type::LSET:
      case Stmt::Type::RSET:
         exe_lrset(static_cast<LRSet *>(s));
         break;

      case Stmt::Type::SLEEP:
         evalPop(static_cast<XSleep *>(s)->ticks);
         m_device.sleep(static_cast<int>(m_top.rval));
         break;

      case Stmt::Type::CLOSE:
         if (!m_files[static_cast<Close *>(s)->fnum].isOpen()) {
            rerror("File not open: CLOSE #%i", static_cast<Close *>(s)->fnum + 1);
         }
         m_files[static_cast<Close *>(s)->fnum].close();
         break;

      case Stmt::Type::OPEN:
         exe_open(static_cast<Open *>(s));
         break;

      case Stmt::Type::WRITE:
         exe_write(static_cast<Write *>(s));
         break;

      case Stmt::Type::FINPUT:
         exe_finput(static_cast<FInput *>(s));
         break;

      case Stmt::Type::INPUT:
         exe_input(static_cast<Input *>(s));
         break;

      case Stmt::Type::FIELD:
         exe_field(static_cast<Field *>(s));
         break;

      case Stmt::Type::GET:
         exe_get(static_cast<GetPut *>(s));
         break;

      case Stmt::Type::PUT:
         exe_put(static_cast<GetPut *>(s));
         break;

      case Stmt::Type::DRAW:
         exe_draw(static_cast<Draw *>(s));
         break;

      case Stmt::Type::LINE:
         exe_line(static_cast<Line *>(s));
         break;

      case Stmt::Type::BOX:
         exe_box(static_cast<Box *>(s));
         break;

      case Stmt::Type::CIRCLE:
         exe_circle(static_cast<Circle *>(s));
         break;

      case Stmt::Type::ELLIPSE:
         exe_ellipse(static_cast<Ellipse *>(s));
         break;

      case Stmt::Type::PAINT:
         exe_paint(static_cast<XPaint *>(s));
         break;

      default:
         assert(0);
      }
      s = s->next;
   } while (false);

   return nullptr != (m_current = s);
}

inline void GVB::exe_dim(Dim *d1) {
   Id *id1 = d1->id;

   switch (id1->type) {
   case Expr::Type::ID: {
      if (m_envVar.count(id1->id)) {
         rerror("Redefined variable: %s", id1->id);
      }

      switch (id1->vtype) {
      case Value::Type::INT:
         m_envVar[id1->id] = 0;
         break;
      case Value::Type::REAL:
         m_envVar[id1->id] = 0.0;
         break;
      case Value::Type::STRING:
         m_envVar[id1->id] = "";
         break;
      default:
         assert(0);
      }
      break;
   }

   case Expr::Type::ARRAYACCESS: {
      ArrayAccess *ac1 = static_cast<ArrayAccess *>(id1);
      if (m_envArray.count(id1->id)) {
         rerror("Redefined array: %s", id1->id);
      }

      Array &a1 = m_envArray[id1->id];
      unsigned total = 1;
      for (auto i : ac1->indices) {
         evalPop(i);
         if (m_top.rval < 0 || m_top.rval >= UINT16_MAX + 1) {
            rerror("Bad index in DIM array: %s, [%f]", m_top.rval);
         }
         unsigned t = static_cast<unsigned>(m_top.rval) + 1;
         a1.bounds.push_back(t);
         total *= t;
      }

      a1.vals.resize(total);
      break;
   }

   default:
      assert(0);
   }
}

inline void GVB::exe_assign(Assign *a1) {
   auto &val = getValue(a1->id);
   evalPop(a1->val);

   switch (a1->id->vtype) {
   case Value::Type::INT:
      checkIntRange(m_top.rval, "assignment");
      val.ival = static_cast<int>(m_top.rval);
      break;
   case Value::Type::REAL:
      val.rval = m_top.rval;
      break;
   case Value::Type::STRING:
      val.sval = m_top.sval;
      break;
   default:
      assert(0);
   }
}

inline void GVB::exe_for(For *f1) {
   if (m_loops.size()) {
      // 查找是否存在自变量相同的for循环，如果存在则要清除
      auto i = m_loops.end();
      do {
         --i;
         if (i->stmt->type == Stmt::Type::FOR && i->_for.stmt->var == f1->var) {
            // 清空内层循环及本循环
            m_loops.erase(i, m_loops.end());
            break;
         }
      } while (i != m_loops.begin());
   }
   m_loops.push_back(Loop(f1));

   auto &top = m_loops.back();
   top.line = m_line;
   top.label = m_label;

   evalPop(f1->dest);
   top._for.dest = m_top.rval;

   if (f1->step) {
      evalPop(f1->step);
      top._for.step = m_top.rval;
   } else {
      top._for.step = 1.0;
   }

   /* for至少会执行一次
    * 如for i = 1 to 0: print i: next
    * 则print i会执行一次，for循环结束后i的值为2
    * */
}

inline Stmt *GVB::exe_next(Next *n1) {
   // 查找自变量相同的for，如果内层有其他循环，全部清除
   while (m_loops.size()) {
      auto &i = m_loops.back();
      if (i.stmt->type == Stmt::Type::FOR
            && (n1->var.empty() || i._for.stmt->var == n1->var))
         break;
      m_loops.pop_back();
   }

   if (m_loops.empty()) {
      rerror("Next without for: NEXT %s", n1->var);
   }
   auto &top = m_loops.back();

   // 修改自变量
   double r1; // 修改后的自变量值
   auto &var1 = m_envVar[top._for.stmt->var];
   switch (Compiler::getIdType(top._for.stmt->var)) {
   case Value::Type::INT:
      r1 = var1.ival + top._for.step;
      checkIntRange(r1, "For");
      var1.ival = static_cast<int>(r1);
      break;
   case Value::Type::REAL:
      r1 = var1.rval += top._for.step; // 不考虑double的溢出
      break;
   default:
      assert(0);
   }

   // 判断for结束条件
   if (top._for.step >= 0 ? r1 > top._for.dest : (r1 < top._for.dest)) {
      m_loops.pop_back();
      return n1;
   }
   m_line = top.line;
   m_label = top.label;
   return top.stmt;
}

inline Stmt *GVB::exe_while(While *w1) {
   if (m_loops.size()) {
      // 查找是否存在同一个while循环，如果内层有其他循环，全部清除
      auto i = m_loops.end();
      do {
         --i;
         if (i->stmt == w1) {
            m_loops.erase(i, m_loops.end()); // 清空内层循环及本循环
            break;
         }
      } while (i != m_loops.begin());
   }

   evalPop(w1->cond);
   if (0 == m_top.rval) {
      // 查找对应的wend。
      // 直接顺序查找，忽略while和wend之间的goto：
      // 30 while 0
      // 40 goto 20  // 会忽略这里的goto，直接跳到50行的wend之后
      // 50 wend
      Stmt *s = w1->next;
      int level = 0;
      while (s) {
         if (s->type == Stmt::Type::WEND) {
            if (!level)
               return s->next;
            --level;
         } else if (s->type == Stmt::Type::WHILE) {
            ++level;
         }
         s = s->next;
      }
      return nullptr;
   } else {
      m_loops.push_back(Loop(w1));
      return w1->next;
   }
}

inline Stmt *GVB::exe_wend() {
   while (m_loops.size()) {
      if (m_loops.back().stmt->type == Stmt::Type::WHILE)
         break;
      m_loops.pop_back();
   }
   if (m_loops.empty()) {
      rerror("Wend withour while");
   }

   Stmt *w1 = m_loops.back().stmt;
   m_loops.pop_back();
   return w1;
}

inline void GVB::exe_print(Print *p1) {
   for (auto &i : p1->exprs) {
      if (i.first) {
         if (i.first->type == Expr::Type::FUNCCALL
                  && static_cast<FuncCall *>(i.first)->isPrintFunc()) {
            auto fc1 = static_cast<FuncCall *>(i.first);
            switch (fc1->ftype) {
            case Func::Type::TAB: {
               evalPop(fc1->expr1);
               if (m_top.rval < 1 || m_top.rval >= 21) {
                  rerror("Illegal argument in PRINT: TAB(%f)", m_top.rval);
               }
               int tab = static_cast<int>(m_top.rval);
               if (m_device.getX() >= tab)
                  m_device.nextRow();
               m_device.locate(m_device.getY(), static_cast<uint8_t>(tab - 1));
               break;
            }

            case Func::Type::SPC:
               evalPop(fc1->expr1);
               if (m_top.rval < 0 || m_top.rval >= UINT8_MAX + 1) {
                  rerror("Illegal argument in PRINT: SPC(%f)", m_top.rval);
               }
               m_device.appendText(string(static_cast<size_t>(m_top.rval), ' '));
               break;

            default:
               assert(0);
            }
         } else {
            evalPop(i.first);
            if (i.first->vtype == Value::Type::STRING) {
               m_device.appendText(removeAllOf(m_top.sval, "\x1f\0", 2));
            } else {
               char buf[50];
               sprintf(buf, "%.9G", m_top.rval);
               m_device.appendText(buf);
            }
         }
      }

      // 显示的文字刚好填满一行的时候就不用换行了
      if (i.second == Print::Delimiter::CR && m_device.getX() > 0) {
         m_device.nextRow();
      }
   }

   m_device.updateLCD();
}

inline Stmt *GVB::exe_if(If *if1) {
   evalPop(if1->cond);
   if (0. != m_top.rval)
      return if1->stmThen;
   else if (if1->stmElse)
      return if1->stmElse;
   return if1->next;
}

inline Stmt *GVB::exe_on(On *on1) {
   evalPop(on1->cond);
   if (m_top.rval >= 1 && m_top.rval <= on1->addrs.size()) {
      if (on1->isSub) {
         if (m_subs.size() >= UINT16_MAX) {
            rerror("Stack overflow in ON-GOSUB");
         }
         m_subs.push_back(Sub(m_line, m_label, on1->next));
      }

      return on1->addrs[static_cast<size_t>(m_top.rval - 1)].stm;
   }
   return on1->next;
}

inline void GVB::exe_locate(Locate *lc1) {
   uint8_t row;
   if (lc1->row) {
      evalPop(lc1->row);
      if (m_top.rval < 1 || m_top.rval >= 6) {
         rerror("Illegal argument in LOCATE: row=%f", m_top.rval);
      }
      row = static_cast<uint8_t>(m_top.rval - 1);
   } else {
      row = m_device.getY();
   }

   evalPop(lc1->col);
   if (m_top.rval < 1 || m_top.rval >= 20) {
      rerror("Illegal argument in LOCATE: col=%f", m_top.rval);
   }
   uint8_t col = static_cast<uint8_t>(m_top.rval - 1);
   if (nullptr == lc1->row && m_device.getX() > col) {
      m_device.nextRow();
   }

   m_device.locate(row, col);
}

inline void GVB::exe_poke(Poke *p1) {
   evalPop(p1->addr);
   if (m_top.rval < 0 || m_top.rval >= UINT16_MAX + 1) {
      rerror("Illegal address in POKE: %f", m_top.rval);
   }
   uint16_t addr = static_cast<uint16_t>(m_top.rval);

   evalPop(p1->val);
   if (m_top.rval < 0 || m_top.rval >= UINT8_MAX + 1) {
      rerror("Illegal value in POKE: %f", m_top.rval);
   }
   m_device.poke(addr, static_cast<uint8_t>(m_top.rval));
}

inline void GVB::exe_call(Call *c1) {
   evalPop(c1->addr);
   if (m_top.rval < 0 || m_top.rval >= UINT16_MAX + 1) {
      rerror("Illegal address in CALL: %f", m_top.rval);
   }
   m_device.call(static_cast<uint16_t>(m_top.rval));
}

inline void GVB::exe_swap(Swap *s1) {
   auto &val1 = getValue(s1->id1);
   auto &val2 = getValue(s1->id2);

   switch (s1->id1->vtype) {
   case Value::Type::INT: {
      int tmp = val1.ival;
      val1.ival = val2.ival;
      val2.ival = tmp;
      break;
   }

   case Value::Type::REAL: {
      double tmp = val1.rval;
      val1.rval = val2.rval;
      val2.rval = tmp;
      break;
   }

   case Value::Type::STRING:
      val1.sval.swap(val2.sval);
      break;

   default:
      assert(0);
   }
}

inline void GVB::exe_read(Read *r1) {
   for (auto i : r1->ids) {
      if (m_dataMan.reachesEnd()) {
         rerror("Out of data: %s", i->id);
      }

      auto &val = getValue(i);
      switch (i->vtype) {
      case Value::Type::INT:
         try {
            val.ival = stoi(m_dataMan.get());
            if (val.ival < INT16_MIN || val.ival > INT16_MAX)
               throw out_of_range("");
         } catch (invalid_argument &) {
            val.ival = 0;
         } catch (out_of_range &) {
            rerror("Integer overflow in READ");
         }
         break;

      case Value::Type::REAL: {
         auto res = RealHelper::validate(val.rval = str2d(m_dataMan.get()));
         assert(res != RealHelper::Result::IS_NAN);
         if (RealHelper::Result::IS_INF == res) {
            rerror("Number overflow in READ");
         }
         break;
      }

      case Value::Type::STRING:
         val.sval = m_dataMan.get();
         break;

      default:
         assert(0);
      }

   }
}

inline void GVB::exe_lrset(LRSet *lr1) {
   auto &val = getValue(lr1->id);
   evalPop(lr1->str);

   auto lsize = val.sval.size(), rsize = m_top.sval.size();
   if (Stmt::Type::LSET == lr1->type) {
      if (lsize > rsize) {
         val.sval.replace(0, rsize, m_top.sval);
      } else {
         val.sval.assign(m_top.sval, 0, lsize);
      }
   } else {
      if (lsize > rsize) {
         val.sval.replace(lsize - rsize, rsize, m_top.sval);
      } else {
         val.sval.assign(m_top.sval, rsize - lsize, lsize);
      }
   }
}

inline void GVB::exe_open(Open *o1) {
   if (m_files[o1->fnum].isOpen()) {
      rerror("Reopen file #%i", o1->fnum + 1);
   }

   evalPop(o1->fname);

   string fname = "dat/" + m_top.sval;
   auto sz = fname.size();
   if (sz < 8 || (fname.back() | 32) != 't'
         || (fname[sz - 2] | 32) != 'a' || (fname[sz - 3] | 32) != 'd'
         || (fname[sz - 4] | 32) != '.') {
      fname += ".DAT";
   }

   if (!m_files[o1->fnum].open(fname, o1->mode)) {
      rerror("File open error: %s", fname);
   }

   if (File::Mode::RANDOM == o1->mode) {
      m_records[o1->fnum].len = o1->len;
      m_records[o1->fnum].total = Open::NOLEN;
   }
}

inline void GVB::exe_write(Write *w1) {
   auto &file = m_files[w1->fnum];
   if (!file.isOpen()) {
      rerror("File not open: WRITE #%i", w1->fnum + 1);
   }

   if (file.mode() != File::Mode::OUTPUT
         && file.mode() != File::Mode::APPEND) {
      rerror("File mode error: WRITE #%i", w1->fnum + 1);
   }

   for (size_t i = 0, j = w1->exprs.size(); i < j; ++i) {
      evalPop(w1->exprs[i]);
      if (w1->exprs[i]->vtype == Value::Type::STRING) {
         file.writeByte('"');
         file.writeString(removeAllOf(m_top.sval, "\x31", 1));
         file.writeByte('"');
      } else {
         file.writeReal(m_top.rval);
      }
      file.writeByte(i < j - 1 ? ',' : static_cast<char>(0xff));
   }
}

inline void GVB::exe_finput(FInput *fi1) {
   auto &file = m_files[fi1->fnum];
   if (!file.isOpen()) {
      rerror("File not open: INPUT #%i", fi1->fnum + 1);
   }

   if (file.mode() != File::Mode::INPUT) {
      rerror("File mode error: INPUT #%i", fi1->fnum + 1);
   }

   for (auto i : fi1->ids) {
      auto &val = getValue(i);
      if (file.eof()) {
         rerror("EOF reached: INPUT #%i", fi1->fnum + 1);
      }

      switch (i->vtype) {
      case Value::Type::INT: {
         double tmp = file.readReal();
         checkIntRange(tmp, "INPUT");
         val.ival = static_cast<int>(tmp);
         break;
      }

      case Value::Type::REAL: {
         auto res = RealHelper::validate(val.rval = file.readReal());
         assert(res != RealHelper::Result::IS_NAN);
         if (res == RealHelper::Result::IS_INF) {
            rerror("Number overflow in INPUT");
         }
         break;
      }

      case Value::Type::STRING:
         val.sval = file.readString();
         break;

      default:
         assert(0);
      }

      file.skip(1); // 跳过','或0xff
   }
}

inline void GVB::exe_input(Input *i1) {
   m_device.appendText(i1->prompt);
   m_device.updateLCD();
   for (auto i : i1->ids) {
      auto &val = getValue(i);

      switch (i->vtype) {
      case Value::Type::INT:
         try {
            val.ival = stoi(m_device.input());
            if (val.ival < INT16_MIN || val.ival > INT16_MAX)
               throw out_of_range("");
         } catch (invalid_argument &) {
            val.ival = 0;
         } catch (out_of_range &) {
            rerror("Integer overflow in INPUT");
         }
         break;

      case Value::Type::REAL: {
         auto res = RealHelper::validate(val.rval = str2d(m_device.input()));
         assert(res != RealHelper::Result::IS_NAN);
         if (RealHelper::Result::IS_INF == res) {
            rerror("Number overflow in INPUT");
         }
         break;
      }

      case Value::Type::STRING:
         val.sval = m_device.input();
         break;

      default:
         assert(0);
      }
   }
}

inline void GVB::exe_field(Field *f1) {
   auto &file = m_files[f1->fnum];
   if (!file.isOpen()) {
      rerror("File not open: FIELD #%i", f1->fnum + 1);
   }

   if (file.mode() != File::Mode::RANDOM) {
      rerror("File mode error: FIELD #%i", f1->fnum + 1);
   }

   auto &rec = m_records[f1->fnum];
   if (rec.len != Open::NOLEN && rec.len < f1->total) {
      rerror("Record size overflow: FIELD #%i: %i, LEN=%i",
             f1->fnum + 1, f1->total, rec.len);
   }

   rec.fields = f1->fields;
   rec.total = f1->total;
   if (rec.len == Open::NOLEN) {
      rec.len = rec.total;
   }

   for (auto &i : rec.fields)
      m_envVar[i.second].sval.assign(static_cast<size_t>(i.first), '\0');
}

inline void GVB::exe_put(GetPut *gp1) {
   auto &file = m_files[gp1->fnum];
   if (!file.isOpen()) {
      rerror("File not open: PUT #%i", gp1->fnum + 1);
   }

   if (file.mode() != File::Mode::RANDOM) {
      rerror("File mode error: PUT #%i", gp1->fnum + 1);
   }

   auto &rec = m_records[gp1->fnum];
   if (rec.total == Open::NOLEN) {
      rerror("Record not assigned: PUT #%i", gp1->fnum + 1);
   }

   evalPop(gp1->record);
   int irec;
   if (m_top.rval < 1 || m_top.rval >= UINT16_MAX
          || (irec = static_cast<int>(m_top.rval - 1)) * rec.len > file.size()) {
      rerror("Bad record number: PUT #%i, %f", gp1->fnum + 1, m_top.rval);
   }

   file.seek(irec * rec.len);
   for (auto &i : rec.fields) {
      file.writeString(m_envVar[i.second].sval);
   }

   if (rec.len > rec.total) {
      for (int i = rec.len - rec.total; i > 0; --i) {
         file.writeByte('\0');
      }
   }
}

inline void GVB::exe_get(GetPut *gp1) {
   auto &file = m_files[gp1->fnum];
   if (!file.isOpen()) {
      rerror("File not open: GET #%i", gp1->fnum + 1);
   }

   if (file.mode() != File::Mode::RANDOM) {
      rerror("File mode error: GET #%i", gp1->fnum + 1);
   }

   auto &rec = m_records[gp1->fnum];
   if (rec.total == Open::NOLEN) {
      rerror("Record not assigned: GET #%i", gp1->fnum + 1);
   }

   evalPop(gp1->record);
   int irec;
   if (m_top.rval < 1 || m_top.rval >= UINT16_MAX
       || (irec = static_cast<int>(m_top.rval)) * rec.len > file.size()) {
      rerror("Bad record number: GET #%i, %f", gp1->fnum + 1, m_top.rval);
   }
   --irec;

   file.seek(irec * rec.len);
   for (auto &i : rec.fields) {
      m_envVar[i.second].sval = file.readString(static_cast<size_t>(i.first));
   }

   if (rec.len > rec.total) {
      file.skip(static_cast<size_t>(rec.len - rec.total));
   }
}

inline uint8_t GVB::getCoord(Expr *coord, const char *x, const char *p) {
   evalPop(coord);
   if (m_top.rval < 0) {
      rerror("Illegal %S value in %S: %f", x, p, m_top.rval);
   }
   if (m_top.rval >= UINT8_MAX + 1)
      return UINT8_MAX;
   return static_cast<uint8_t>(m_top.rval);
}

inline Device::DrawMode GVB::getDrawMode(Expr *e1) {
   if (nullptr == e1)
      return Device::PAINT;
   evalPop(e1);
   return static_cast<Device::DrawMode>(static_cast<int>(m_top.rval)
                                        & Device::DRAW_MASK);
}

inline bool GVB::getFillType(Expr *e1) {
   if (nullptr == e1)
      return false;
   evalPop(e1);
   return m_top.rval != 0.;
}

inline void GVB::exe_draw(Draw *d1) {
   m_device.point(getCoord(d1->x, "x", "DRAW"),
                  getCoord(d1->y, "y", "DRAW"),
                  getDrawMode(d1->ctype));
}

inline void GVB::exe_line(Line *l1) {
   m_device.line(getCoord(l1->x1, "x1", "LINE"),
                 getCoord(l1->y1, "y1", "LINE"),
                 getCoord(l1->x2, "x2", "LINE"),
                 getCoord(l1->y2, "y2", "LINE"),
                 getDrawMode(l1->ctype));
}

inline void GVB::exe_box(Box *b1) {
   m_device.rectangle(getCoord(b1->x1, "x1", "BOX"),
                      getCoord(b1->y1, "y1", "BOX"),
                      getCoord(b1->x2, "x2", "BOX"),
                      getCoord(b1->y2, "y2", "BOX"),
                      getFillType(b1->fill),
                      getDrawMode(b1->ctype));
}

inline void GVB::exe_circle(Circle *c1) {
   auto r = getCoord(c1->radius, "radius", "CIRCLE");
   m_device.ellipse(getCoord(c1->x, "x", "CIRCLE"),
                    getCoord(c1->y, "y", "CIRCLE"),
                    r, r,
                    getFillType(c1->fill),
                    getDrawMode(c1->ctype));
}

inline void GVB::exe_ellipse(Ellipse *c1) {
   m_device.ellipse(getCoord(c1->x, "x", "ELLIPSE"),
                    getCoord(c1->y, "y", "ELLIPSE"),
                    getCoord(c1->rx, "x-radius", "ELLIPSE"),
                    getCoord(c1->ry, "y-radius", "ELLIPSE"),
                    getFillType(c1->fill),
                    getDrawMode(c1->ctype));
}

inline void GVB::exe_paint(XPaint *p1) {
   evalPop(p1->addr);
   uint16_t addr = static_cast<uint16_t>(m_top.rval);

   evalPop(p1->x);
   int x = static_cast<int>(m_top.rval);

   evalPop(p1->y);
   int y = static_cast<int>(m_top.rval);

   evalPop(p1->w);
   uint8_t w = static_cast<uint8_t>(m_top.rval);

   evalPop(p1->h);
   uint8_t h = static_cast<uint8_t>(m_top.rval);

   Device::PaintMode mode;
   if (nullptr == p1->mode)
      mode = Device::PaintMode::COPY;
   else {
      evalPop(p1->mode);
      mode = static_cast<Device::PaintMode>(static_cast<int>(m_top.rval)
            % static_cast<int>(Device::PaintMode::DUMMY));
   }

   m_device.paint(addr, x, y, w, h, mode);
}

inline void GVB::evalPop(Expr *e1) {
   eval(e1);
   m_top = m_stack.back();
   m_stack.pop_back();
}

void GVB::eval(Expr *e1) {
   switch (e1->type) {
   case Expr::Type::ID:
   case Expr::Type::ARRAYACCESS: {
      auto &val = getValue(static_cast<Id *>(e1));

      if (Value::Type::INT == e1->vtype)
         m_stack.push_back(Single(static_cast<double>(val.ival)));
      else
         m_stack.push_back(val);
      break;
   }

   case Expr::Type::REAL:
      // 编译时确保rval一定有效，不需要再检查
      m_stack.push_back(Single(static_cast<Real *>(e1)->rval));
      break;

   case Expr::Type::STRING:
      m_stack.push_back(Single(static_cast<Str *>(e1)->sval));
      break;

   case Expr::Type::INKEY:
      m_stack.push_back(Single(string(1u, static_cast<char>(m_device.getKey()))));
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

// 会占用m_top
GVB::Single &GVB::getValue(Id *id1) {
   if (id1->type == Expr::Type::ID) { // 变量
      return m_envVar[id1->id];
   } else { // 数组
      auto ac1 = static_cast<ArrayAccess *>(id1);
      auto &a1 = m_envArray[id1->id];

      if (a1.bounds.empty()) { // 数组不存在
         // 初始化数组
         a1.bounds.resize(ac1->indices.size(), 11u);
         unsigned total = 1;
         for (auto j = ac1->indices.size(); j > 0; --j)
            total *= 11;

         a1.vals.resize(total);
      }

      if (ac1->indices.size() != a1.bounds.size()) {
         rerror("Array dimension mismatch: expecting %i, got %i",
                static_cast<int>(a1.bounds.size()),
                static_cast<int>(ac1->indices.size()));
      }

      evalPop(ac1->indices[0]);
      if (m_top.rval < 0 || m_top.rval >= a1.bounds[0]) {
         rerror("Bad index in array: %s 0[%f]", ac1->id, m_top.rval);
      }

      unsigned total = static_cast<unsigned>(m_top.rval);
      for (size_t i = 1; i < ac1->indices.size(); ++i) {
         evalPop(ac1->indices[i]);
         if (m_top.rval < 0 || m_top.rval >= a1.bounds[i]) {
            rerror("Bad index in array: %s %i[%f]", ac1->id,
                   static_cast<int>(i), m_top.rval);
         }
         total = total * a1.bounds[i] + static_cast<unsigned>(m_top.rval);
      }

      return a1.vals[total];
   }
}

inline void GVB::eval_binary(Binary *b1) {
   eval(b1->left);
   evalPop(b1->right);

   switch (b1->op) {
   case '+':
      if (Value::Type::REAL == Compiler::getRValType(b1->right->vtype)) {
         auto res = RealHelper::validate(m_stack.back().rval += m_top.rval);
         assert(res != RealHelper::Result::IS_NAN); // 加法会nan ?
         if (RealHelper::Result::IS_INF == res) {
            rerror("Number overflow in +");
         }
      } else
         m_stack.back().sval += m_top.sval;
      break;

   case '-': {
      auto res = RealHelper::validate(m_stack.back().rval -= m_top.rval);
      assert(res != RealHelper::Result::IS_NAN);
      if (RealHelper::Result::IS_INF == res) {
         rerror("Number overflow in -");
      }
      break;
   }

   case '*': {
      auto res = RealHelper::validate(m_stack.back().rval *= m_top.rval);
      assert(res != RealHelper::Result::IS_NAN);
      if (RealHelper::Result::IS_INF == res) {
         rerror("Number overflow in *");
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
         rerror("Number overflow in /");
      }
      break;
   }

   case '^': {
      errno = 0;
      double r = pow(m_stack.back().rval, m_top.rval);
      int err = errno;
      auto res = RealHelper::validate(m_stack.back().rval = r);
      if (ERANGE == err || RealHelper::Result::IS_INF == res) {
         rerror("Number overflow in ^");
      }
      if (err || RealHelper::Result::IS_VALID != res) {
         rerror("Illegal operand in ^");
      }
      break;
   }

#define CMP(op)    \
   if (Value::Type::REAL == Compiler::getRValType(b1->right->vtype)) { \
      m_stack.back().rval = m_stack.back().rval op m_top.rval; \
   } else { \
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
      m_stack.back().rval = m_stack.back().sval[0] & 0xff;
      break;

   case Func::Type::ATN: {
      auto res = RealHelper::validate(m_stack.back().rval =
                                            atan(m_stack.back().rval));
      assert(res == RealHelper::Result::IS_VALID);
      break;
   }

   case Func::Type::CHR: {
      if (m_stack.back().rval < 0 || m_stack.back().rval >= 256) {
         rerror("Illegal argument: CHR$(%f)", m_stack.back().rval);
      }

      char c = static_cast<char>(static_cast<unsigned>(m_stack.back().rval));
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

   case Func::Type::TAN: {
      auto res = RealHelper::validate(m_stack.back().rval =
                                            tan(m_stack.back().rval));
      assert(res != RealHelper::Result::IS_NAN);
      if (RealHelper::Result::IS_INF == res) {
         rerror("Number overflow in TAN");
      }
      break;
   }

   case Func::Type::CVI: // 2-byte => int
      if (m_stack.back().sval.size() < 2) {
         rerror("String too short in CVI: LEN(%s)=%i",
                m_stack.back().sval,
                static_cast<int>(m_stack.back().sval.size()));
      }
      m_stack.back().rval = *reinterpret_cast<const int16_t *>(
            m_stack.back().sval.data());
      break;

   case Func::Type::MKI: { // int => 2-byte
      if (m_stack.back().rval < INT16_MIN || m_stack.back().rval >= INT16_MAX + 1) {
         rerror("Illegal argument in MKI: %f",
                m_stack.back().rval);
      }
      int16_t s1 = static_cast<int16_t>(m_stack.back().rval);
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
      m_stack.back().rval = RealHelper::toDouble(
            *reinterpret_cast<const uint64_t *>(m_stack.back().sval.data()));
      break;

   case Func::Type::MKS: { // double => 5-byte
      uint64_t d = RealHelper::fromDouble(m_stack.back().rval);
      m_stack.back().sval.assign(reinterpret_cast<char *>(&d),
                                 reinterpret_cast<char *>(&d) + 5);
      break;
   }

   case Func::Type::EXP: {
      auto res = RealHelper::validate(m_stack.back().rval =
                                            exp(m_stack.back().rval));
      assert(res != RealHelper::Result::IS_NAN);
      if (RealHelper::Result::IS_NAN == res) {
         rerror("Number overflow in EXP");
      }
      break;
   }

   case Func::Type::INT:
      m_stack.back().rval = floor(m_stack.back().rval); // 需要validate ?
      break;

   case Func::Type::LEN:
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
         rerror("Number overflow in LOG");
      }
      if (err || RealHelper::Result::IS_VALID != res) {
         rerror("Illegal argument in LOG");
      }
      break;
   }

   case Func::Type::MID: {
      evalPop(fc->expr2);
      auto &s = m_stack.back().sval;
      if (m_top.rval < 1 || m_top.rval >= s.size() + 1) {
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
      if (offset + m_top.rval >= s.size() + 1)
         count = static_cast<unsigned>(s.size() - offset);
      s = s.substr(offset, count);
      break;
   }

   case Func::Type::POS:
      m_stack.back().rval = m_device.getX() + 1;
      break;

   case Func::Type::RND: {
      m_stack.back().rval = (0. == m_stack.back().rval ? m_rand.stationary()
            : m_stack.back().rval > 0. ? m_rand.random() : m_rand.sequence())
            / static_cast<double>(Random::MAX + 1);
      auto res = RealHelper::validate(m_stack.back().rval);
      assert(res == RealHelper::Result::IS_VALID);
      break;
   }

   case Func::Type::SGN: {
      auto &r = m_stack.back().rval;
      if (r < 0.0)
         r = -1.0;
      else if (r > 0.0)
         r = 1.0;
      break;
   }

   case Func::Type::SQR: {
      double &r = m_stack.back().rval;
      errno = 0;
      r = sqrt(r);
      if (errno) {
         rerror("Illegal argument in SQR");
      }
      auto res = RealHelper::validate(r);
      assert(res == RealHelper::Result::IS_VALID);
      break;
   }

   case Func::Type::STR: {
      char buf[100];
      sprintf(buf, "%.9G", m_stack.back().rval);
      m_stack.back().sval = buf;
      break;
   }

   case Func::Type::VAL: {
      auto res = RealHelper::validate(m_stack.back().rval = str2d(
            m_stack.back().sval));
      assert(res != RealHelper::Result::IS_NAN);
      if (RealHelper::Result::IS_INF == res) {
         rerror("Number overflow in VAL");
      }
      break;
   }

   case Func::Type::PEEK:
      if (m_stack.back().rval < 0 || m_stack.back().rval >= UINT16_MAX + 1) {
         rerror("Illegal argument: PEEK(%f)", m_stack.back().rval);
      }
      m_stack.back().rval = m_device.peek(
            static_cast<uint16_t>(m_stack.back().rval));
      break;

   case Func::Type::FEOF: {
      if (m_stack.back().rval < 1 || m_stack.back().rval > 3) {
         rerror("Illegal file number: EOF(%f)", m_stack.back().rval);
      }

      int fnum = static_cast<int>(m_stack.back().rval) - 1;
      if (!m_files[fnum].isOpen()) {
         rerror("File not open: EOF(%i)", fnum + 1);
      }

      if (m_files[fnum].mode() != File::Mode::INPUT) {
         rerror("File mode error: EOF(%i), mode:%m", fnum + 1, m_files[fnum].mode());
      }

      m_stack.back().rval = static_cast<double>(m_files[fnum].eof());
      break;
   }

   case Func::Type ::LOF: {
      if (m_stack.back().rval < 1 || m_stack.back().rval >= 4) {
         rerror("Illegal file number: LOF(%f)", m_stack.back().rval);
      }

      int fnum = static_cast<int>(m_stack.back().rval) - 1;
      if (!m_files[fnum].isOpen()) {
         rerror("File not open: LOF(%i)", fnum + 1);
      }

      if (m_files[fnum].mode() != File::Mode::RANDOM) {
         rerror("File mode error: LOF(%i), mode:%m", fnum + 1, m_files[fnum].mode());
      }

      m_stack.back().rval = static_cast<double>(m_files[fnum].size());
      break;
   }

   default:
      assert(0);
   }
}

inline void GVB::eval_usercall(UserCall *uc1) {
   auto f1 = m_funcs[uc1->fnName];
   if (nullptr == f1) {
      rerror("Function undefined: %s", uc1->fnName);
   }

   if (Compiler::getIdRValType(f1->varName) != uc1->expr->vtype) {
      rerror("Incompatible type in FN %s(%s: %t): [%t]",
             uc1->fnName, f1->varName, Compiler::getIdRValType(f1->varName),
             uc1->expr->vtype);
   }

   evalPop(uc1->expr);

   auto &var = m_envVar[f1->varName];
   switch (Compiler::getIdType(f1->varName)) {
   case Value::Type::INT:
      checkIntRange(m_top.rval, "User-call");
      var.ival = static_cast<int>(m_top.rval);
      break;
   case Value::Type::REAL:
      var.rval = m_top.rval;
      break;
   case Value::Type::STRING:
      var.sval = m_top.sval;
      break;
   default:
      assert(0);
   }

   eval(f1->fn);
   if (Compiler::getIdType(f1->fnName) == Value::Type::INT) {
      checkIntRange(m_stack.back().rval, "User-call");
   }
}

inline void GVB::checkIntRange(double r, const char *s) {
   if (r < INT16_MIN || r >= INT16_MAX + 1) {
      rerror("Integer overflow in %S: %f", r, s);
   }
}
