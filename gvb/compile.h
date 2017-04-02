#ifndef GVBASIC_COMPILE_H
#define GVBASIC_COMPILE_H

#include <unordered_map>
#include <vector>
#include <tuple>
#include "lex.h"
#include "value.h"

namespace gvbsim {

class NewLine;
class Stmt;
class NodeManager;
class DataManager;
class Expr;


class Compiler {
   Lexer m_l;
   int m_line, m_label; // 当前行, 标号
   int m_tok;
   std::unordered_map<int, NewLine *> m_labels; // 标号 -> 一行的第一条语句(总是NewLine)
   std::vector<std::tuple<Stmt *, int, int>> m_refs; // 待回填的标号引用

   NodeManager &m_nodeMan;
   DataManager &m_dataMan;

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
   void resolveRefs();

   Expr *expr(Value::Type);
};

}

#endif //GVBASIC_COMPILE_H
