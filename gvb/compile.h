#ifndef GVBASIC_COMPILE_H
#define GVBASIC_COMPILE_H

#include <unordered_map>
#include <vector>
#include "lex.h"

namespace gvbsim {

class NewLine;
class Stmt;
class NodeManager;

class Compiler {
   Lexer l;
   int line, label; // 当前行, 标号
   int tok;
   std::unordered_map<int, NewLine *> labels; // 标号 -> 一行的第一条语句(总是NewLine)
   std::vector<Stmt *> refs; // 待回填的标号引用

   NodeManager &nodeMan;

public:
   Compiler(FILE *, NodeManager &);

public:
   Stmt *compile();

private:
   void peek();
   void match(int);
};

}

#endif //GVBASIC_COMPILE_H
