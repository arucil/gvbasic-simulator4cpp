#ifndef GVBASIC_COMPILE_H
#define GVBASIC_COMPILE_H

#include <unordered_map>
#include <vector>
#include <tuple>
#include "lex.h"
#include "value.h"
#include "func.h"

namespace gvbsim {

class NewLine;
class Stmt;
class NodeManager;
class DataManager;
class Expr;
class Id;
class Goto;
class Restore;
class On;


class Compiler {
   Lexer m_l;
   int m_line, m_label; // 当前行, 标号
   int m_tok;
   std::unordered_map<int, NewLine *> m_labels; // 标号 -> 一行的第一条语句(总是NewLine)
   std::vector<std::tuple<Stmt *, int, int>> m_refs; // 待回填的标号引用, line, label

   NodeManager &m_nodeMan;
   DataManager &m_dataMan;

   const static std::unordered_map<std::string, Func::Type> s_builtinFuncs;

public:
   Compiler(FILE *, NodeManager &, DataManager &);

public:
   Stmt *compile();

private:
   void peek();
   void match(int);

   Stmt *getStmtTail(Stmt *);
   void linkStmt(Stmt *, Stmt *);
   NewLine *aLine();
   Stmt *stmts();
   Stmt *stmt(bool inIf);
   Stmt *ifstmt();
   Stmt *ifclause();
   void datastmt();
   Stmt *defstmt();
   Stmt *printstmt();
   Stmt *forstmt();
   Stmt *nextstmt();
   Stmt *dimstmt();
   Stmt *inputstmt();
   Stmt *writestmt();
   Stmt *readstmt();
   Stmt *openstmt();
   Stmt *onstmt();
   Stmt *fieldstmt();
   void resolveRefs();
   Stmt *findLabel(Goto *);
   Stmt *findLabel(Restore *);
   Stmt *findLabel(On *);
   int getFileNum();

   Stmt *translate(Stmt *, Stmt *end = nullptr);

   Expr *expr(Value::Type);
   Expr *expr();
   Expr *E_(int prec);
   Expr *F();
   Expr *idexpr();
   Id *getId();

   static int getp(int op);
   static Value::Type getIdType(const std::string &);
   static Value::Type getIdRValType(const std::string &);
   static Value::Type getRValType(Value::Type);
   static std::string &addCNPrefix(std::string &);
   static std::string &rtrim(std::string &);
};

}

#endif //GVBASIC_COMPILE_H
