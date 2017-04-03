#ifndef GVBASIC_TREE_H
#define GVBASIC_TREE_H

#include <utility>
#include <string>
#include <vector>
#include "value.h"
#include "node.h"
#include "func.h"

namespace gvbsim {

struct Stmt : Node { // 语句
   enum class Type {
      NONE, CLS, DIM, ASSIGN, SWAP, IF, ON, FOR, NEXT, WHILE, WEND,
      GOSUB, RETURN, READ, RESTORE, INPUT, FINPUT, PRINT, LOCATE, INKEY,
      GRAPH, TEXT, DRAW, LINE, BOX, CIRCLE, ELLIPSE, OPEN, CLOSE, PUT,
      GET, LSET, RSET, POP, CLEAR, WRITE, POKE, CALL, END, FIELD, GOTO,
      INVERSE, DEFFN, PLAY, BEEP, NEWLINE, SLEEP,
   };

public:
   Type type;
   Stmt * next;

public:
   Stmt(Type type)
         : type(type), next(nullptr) { }
};

struct Expr : Node { // 表达式
   enum class Type {
      BINARY, FUNCCALL, USERCALL, ID, ARRAYACCESS, INKEY, STRING, REAL
   };

public:
   Type type;
   Value::Type vtype; // 用于类型检查

protected:
   Expr(Type type, Value::Type vtype)
         : type(type), vtype(vtype) { }
};

/*--------------------------- expr ----------------------------*/

struct Binary : Expr { // 二元式
   Expr *left, *right;
   int op;

public:
   Binary(Expr *left, Expr *right, int op, Value::Type vt)
         : Expr(Type::BINARY, vt),
           left(left), right(right), op(op) { }
};

struct Str : Expr { // 字符串常量
   std::string sval;

public:
   Str(const std::string &s)
         : Expr(Type::STRING, Value::Type::STRING),
           sval(s) { }
};

struct Real : Expr { // 实数常量
   double rval;

public:
   Real(double d)
         : Expr(Type::REAL, Value::Type::REAL),
           rval(d) { }
};

struct FuncCall : Expr { // 系统函数调用
   Expr *expr1, *expr2, *expr3;
   Func::Type ftype;

public:
   FuncCall(Func::Type ftype, Value::Type vt, Expr *expr1, Expr *expr2 = nullptr,
            Expr *expr3 = nullptr)
         : Expr(Type::FUNCCALL, vt),
           expr1(expr1), expr2(expr2), expr3(expr3), ftype(ftype) { }
};

struct UserCall : Expr { // 用户自定义函数调用
   Expr *expr;
   std::string fnName;

public:
   UserCall(Expr *expr, const std::string &f, Value::Type vt)
         : Expr(Type::USERCALL, vt),
           expr(expr), fnName(f) { }
};

struct Id : Expr { // 标识符
   std::string id;

public:
   Id(const std::string &id, Value::Type vt)
         : Id(id, Type::ID, vt) { }

protected:
   Id(const std::string &id, Type type, Value::Type vt)
         : Expr(type, vt),
           id(id) { }
};

struct ArrayAccess : Id { // 数组访问
   std::vector<Expr *> indices;

public:
   ArrayAccess(const std::string &id, const std::vector<Expr *> &v,
               Value::Type vt)
         : Id(id, Type::ARRAYACCESS, vt),
           indices(v) { }
};

struct Inkey : Expr { // 按键
   Inkey() : Expr(Type::INKEY, Value::Type::STRING) {}
};


/*----------------------- statements ------------------------*/

struct If : Stmt { // if
   Expr *cond;
   Stmt *stmThen, *stmElse;

public:
   If(Expr *cond, Stmt *stm, Stmt *stmElse)
         : Stmt(Type::IF),
           cond(cond), stmThen(stm), stmElse(stmElse) { }
};

struct Call : Stmt { // call
   Expr *addr;

public:
   Call(Expr *addr)
         : Stmt(Type::CALL),
           addr(addr) { }
};

struct Assign : Stmt { // let
   Id *id;
   Expr *val;

public:
   Assign(Id *id, Expr *val)
         : Stmt(Type::ASSIGN),
           id(id), val(val) { }
};

struct NewLine : Stmt { // newline
   int label;
   int line;

public:
   NewLine(int label, int line)
         : Stmt(Type::NEWLINE),
           label(label), line(line) { }
};

struct Goto : Stmt { // goto / gosub
   union {
      int label;
      NewLine *stm;
   };

public:
   Goto(int label, bool isSub)
         : Stmt(isSub ? Type::GOSUB : Type::GOTO),
           label(label) { }
};

struct DefFn : Stmt { // def fn
   std::string fnName, varName;
   Expr *fn;

public:
   DefFn(const std::string &f, const std::string &var, Expr *fn)
         : Stmt(Type::DEFFN),
           fnName(f), varName(var), fn(fn) { }
};

struct Print : Stmt { // print
   enum class Delimiter {
      NO, CR
   };

public:
   std::vector<std::pair<Expr *, Delimiter>> exprs;

public:
   Print() : Stmt(Type::PRINT) { }
};

struct For : Stmt { // for loop
   std::string var;
   Expr *dest, *step; // step可能为null

public:
   For(const std::string &var, Expr *dest, Expr *step)
         : Stmt(Type::FOR),
           var(var), dest(dest), step(step) { }
};

struct Next : Stmt { // next
   std::string var; // 可能为空

public:
   Next(const std::string &var)
         : Stmt(Type::NEXT),
           var(var) {}
};

struct Dim : Stmt { // dim
   Id *id;

public:
   Dim(Id *id)
         : Stmt(Type::DIM),
           id(id) { }
};

struct While : Stmt { // while loop
   Expr *cond;

public:
   While(Expr *cond)
         : Stmt(Type::WHILE),
           cond(cond) { }
};

struct Input : Stmt { // input [prompt, ] var1, ...
   std::string prompt;
   std::vector<Id *> ids;

public:
   Input(const std::string &s)
         : Stmt(Type::INPUT),
           prompt(s) { }
};

struct FInput : Stmt { // input #n, var1, ...
   int fnum;
   std::vector<Id *> ids;

public:
   FInput(int fnum)
         : Stmt(Type::FINPUT),
           fnum(fnum) { }
};

struct Write : Stmt { // write
   int fnum;
   std::vector<Expr *> exprs;

public:
   Write(int fnum)
         : Stmt(Type::WRITE),
           fnum(fnum) { }
};

struct Close : Stmt { // close
   int fnum;

public:
   Close(int fnum)
         : Stmt(Type::CLOSE),
           fnum(fnum) { }
};

struct Open : Stmt { // open
   enum class Mode {
      INPUT, OUTPUT, RANDOM, APPEND
   };
   enum { NOLEN = -1 };

public:
   int fnum;
   Expr *fname;
   Mode mode;
   int len; // random模式用

public:
   Open(int fnum, Expr *fname, Mode mode, int len)
         : Stmt(Type::OPEN),
           fnum(fnum), fname(fname), mode(mode), len(len) { }
};

struct Locate : Stmt { // locate
   Expr *row, *col; // row可能为null

public:
   Locate(Expr *row, Expr *col)
         : Stmt(Type::LOCATE),
           row(row), col(col) { }
};

struct Swap : Stmt { // swap
   Id *id1, *id2;

public:
   Swap(Id *id1, Id *id2)
         : Stmt(Type::SWAP),
           id1(id1), id2(id2) { }
};

struct Read : Stmt {
   std::vector<Id *> ids;

public:
   Read() : Stmt(Type::READ) { }
};

struct Poke : Stmt {
   Expr *addr, *val;

public:
   Poke(Expr *addr, Expr *val)
         : Stmt(Type::POKE),
           addr(addr), val(val) { }
};

struct Restore : Stmt {
   enum { NO_LABEL = -1 };

public:
   int label;

public:
   Restore(int label)
         : Stmt(Type::RESTORE),
           label(label) { }
};

struct On : Stmt {
   union Addr {
      int label;
      NewLine *stm;
   };

public:
   Expr *cond;
   std::vector<Addr> addrs;
   bool isSub;

public:
   On(Expr *cond, bool isSub)
         : Stmt(Type::ON),
           cond(cond), isSub(isSub) { }
};

struct Draw : Stmt {
   Expr *x, *y, *ctype; // type可能为null

public:
   Draw(Expr *x, Expr *y, Expr *t)
         : Stmt(Type::DRAW),
           x(x), y(y), ctype(t) { }
};

struct Line : Stmt {
   Expr *x1, *y1, *x2, *y2, *ctype;

public:
   Line(Expr *x1, Expr *y1, Expr *x2, Expr *y2, Expr *t)
         : Stmt(Type::LINE),
           x1(x1), y1(y1), x2(x2), y2(y2), ctype(t) { }
};

struct Box : Stmt {
   Expr *x1, *y1, *x2, *y2, *fill, *ctype; // fill, type可能为null

public:
   Box(Expr *x1, Expr *y1, Expr *x2, Expr *y2, Expr *f, Expr *t)
         : Stmt(Type::BOX),
           x1(x1), y1(y1), x2(x2), y2(y2), fill(f), ctype(t) { }
};

struct Circle : Stmt {
   Expr *x, *y, *radius, *fill, *ctype;

public:
   Circle(Expr *x, Expr *y, Expr *r, Expr *f, Expr *t)
         : Stmt(Type::CIRCLE),
           x(x), y(y), radius(r), fill(f), ctype(t) { }
};

struct Ellipse : Stmt {
   Expr *x, *y, *rx, *ry, *fill, *ctype;

public:
   Ellipse(Expr *x, Expr *y, Expr *rx, Expr *ry, Expr *f, Expr *t)
         : Stmt(Type::ELLIPSE),
           x(x), y(y), rx(rx), ry(ry), fill(f), ctype(t) { }
};

struct GetPut : Stmt { // put / get
   int fnum;
   Expr *record;

public:
   GetPut(int fnum, Expr *r, bool isGet)
         : Stmt(isGet ? Type::GET : Type::PUT),
           fnum(fnum), record(r) { }
};

struct Field : Stmt {
   int fnum;
   int total;
   std::vector<std::pair<int, std::string>> fields;

public:
   Field(int fnum)
         : Stmt(Type::FIELD),
           fnum(fnum) { }
};

struct LRSet : Stmt {
   Id *id;
   Expr *str;

public:
   LRSet(Id *id, Expr *str, bool isL)
         : Stmt(isL ? Type::LSET : Type::RSET),
           id(id), str(str) { }
};

struct Play : Stmt {
   Expr *str;

public:
   Play(Expr *str)
         : Stmt(Type::PLAY),
           str(str) { }
};

/*-----------------     extended  ------------------*/

struct XSleep : Stmt {
   Expr *ticks;

public:
   XSleep(Expr *ticks)
         : Stmt(Type::SLEEP),
           ticks(ticks) { }
};

}

#endif //GVBASIC_TREE_H
