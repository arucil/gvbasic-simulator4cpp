#ifndef GVBASIC_GVB_H
#define GVBASIC_GVB_H

#include <vector>
#include <unordered_map>
#include <cstdio>
#include <ctime>
#include "node_man.h"
#include "data_man.h"
#include "random.h"
#include "value.h"
#include "file.h"
#include "device.h"

namespace gvbsim {

class Stmt;
class Dim;
class Assign;
class While;
class DefFn;
class Expr;
class Id;
class For;
class Next;
class ArrayAccess;
class FuncCall;
class Binary;
class UserCall;
class While;
class If;
class On;
class Print;
class Locate;
class Poke;
class Call;
class Swap;
class LRSet;
class Read;
class Open;
class Write;
class Input;
class FInput;
class Field;
class GetPut;
class Draw;
class Line;
class Box;
class Circle;
class Ellipse;
class XPaint;


class GVB {
   friend class Compiler;

   struct Loop {
      int line, label;

      union {
         Stmt *stmt;

         struct {
            For *stmt;
            double dest, step;
         } _for;
      };

   public:
      Loop(Stmt *stmt) : stmt(stmt) { }
   };

   struct Sub {
      int line, label;
      Stmt *stmt; // gosub的下一条语句

      Sub(int line, int label, Stmt *stmt)
            : line(line), label(label), stmt(stmt) { }
   };

   // 单个值
   struct Single {
      union {
         int ival;
         double rval;
      };
      std::string sval;

   public:
      Single() = default; // 换成 {} 不行?
      Single(int ival) : ival(ival) { }
      Single(double rval) : rval(rval) { }
      Single(const std::string &s) : sval(s) { }
      Single(const char *s) : sval(s) { }
   };

   // 数组
   struct Array {
      std::vector<unsigned> bounds;
      std::vector<Single> vals;
   };

   // random文件的记录
   struct Record {
      int len; // open语句定义的LEN
      int total; // field语句定义的len，可能小于上面的len
      std::vector<std::pair<int, std::string>> fields;
   };

private:
   Device m_device;
   Stmt *m_head;
   std::vector<Single> m_stack;
   Single m_top;
   std::vector<Sub> m_subs;
   std::vector<Loop> m_loops;
   std::unordered_map<std::string, DefFn *> m_funcs; // 用户定义函数
   std::unordered_map<std::string, Single> m_envVar;
   std::unordered_map<std::string, Array> m_envArray;
   Random m_rand;
   NodeManager m_nodeMan;
   DataManager m_dataMan;
   File m_files[3];
   Record m_records[3];
   int m_line, m_label;

public:
   GVB();
   ~GVB();

   void build(std::FILE *fp);
   bool isBuilt() const { return static_cast<bool>(m_head); }

   void execute(uint32_t seed = static_cast<uint32_t>(std::time(nullptr)));

private:
   void clearEnv();
   void clearStack();
   void clearFiles();

   void traverse(Stmt *);

   void exe_dim(Dim *);
   void exe_assign(Assign *);
   void exe_for(For *);
   Stmt *exe_next(Next *);
   Stmt *exe_while(While *);
   Stmt *exe_wend();
   void exe_print(Print *);
   Stmt *exe_if(If *);
   Stmt *exe_on(On *);
   void exe_locate(Locate *);
   void exe_poke(Poke *);
   void exe_call(Call *);
   void exe_swap(Swap *);
   void exe_read(Read *);
   void exe_lrset(LRSet *);
   void exe_open(Open *);
   void exe_write(Write *);
   void exe_finput(FInput *);
   void exe_input(Input *);
   void exe_field(Field *);
   void exe_put(GetPut *);
   void exe_get(GetPut *);
   void exe_draw(Draw *);
   void exe_line(Line *);
   void exe_box(Box *);
   void exe_circle(Circle *);
   void exe_ellipse(Ellipse *);
   void exe_paint(XPaint *);

   void eval(Expr *);
   void evalPop(Expr *);
   void eval_func(FuncCall *);
   void eval_binary(Binary *);
   void eval_usercall(UserCall *);

   Single &getValue(Id *);

   void checkIntRange(double r, const char *);
   uint8_t getCoord(Expr *, const char *, const char *);
   Device::DrawMode getDrawMode(Expr *);
   bool getFillType(Expr *);

private:
   static void error(int line, int label, const char *s, ...);

   static std::string &removeAllOf(std::string &, const char *, size_t);

public:
   static double str2d(const std::string &); // 不识别inf和nan
};

}

#endif //GVBASIC_GVB_H
