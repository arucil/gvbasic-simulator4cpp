#ifndef GVBASIC_GVB_H
#define GVBASIC_GVB_H

#include <vector>
#include <unordered_map>
#include <cstdio>
#include "node_man.h"
#include "data_man.h"
#include "random.h"
#include "value.h"
#include "file.h"

namespace gvbsim {

class Device;
class Stmt;
class Dim;
class Assign;
class While;
class DefFn;
class Expr;
class Id;
class ArrayAccess;
class FuncCall;
class Binary;
class UserCall;


class GVB {
   friend class Compiler;

   struct ForLoop {
      double dest;
      double step;
      int line, label;
      Stmt *stmt; // for的下一条语句
   };

   struct WhileLoop {
      int line, label;
      While *stmt;
   };

   struct Sub {
      int line, label;
      Stmt *stmt; // gosub的下一条语句
   };

private:
   Device *const m_device;
   Stmt *m_head;
   std::vector<Single> m_stack;
   Single m_top;
   std::vector<Sub> m_subs;
   std::vector<WhileLoop> m_whiles;
   std::vector<ForLoop> m_fors;
   std::unordered_map<std::string, DefFn *> m_funcs; // 用户定义函数
   std::unordered_map<std::string, Single> m_envVar;
   std::unordered_map<std::string, Array> m_envArray;
   Random m_rand;
   NodeManager m_nodeMan;
   DataManager m_dataMan;
   File m_files[3];
   int m_line, m_label;

public:
   explicit GVB(Device *);
   ~GVB();

   void build(std::FILE *fp);
   bool isBuilt() const { return static_cast<bool>(m_head); }

   void execute();

private:
   void clearEnv();
   void clearStack();
   void clearFiles();

   void traverse(Stmt *);

   void exe_dim(Dim *);
   void exe_assign(Assign *);

   void eval(Expr *);
   void evalPop(Expr *);
   void eval_id(Id *);
   void eval_access(ArrayAccess *);
   void eval_func(FuncCall *);
   void eval_binary(Binary *);
   void eval_usercall(UserCall *);

private:
   static void error(int line, int label, const char *s, ...);
};

}

#endif //GVBASIC_GVB_H
