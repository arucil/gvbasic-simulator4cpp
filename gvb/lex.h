#ifndef GVBASIC_LEX_H
#define GVBASIC_LEX_H

#include <string>
#include <cstdio>
#include <unordered_map>
#include "common.h"

namespace gvbsim {

struct Token {
   enum {
      ID = 256, INT, REAL, STRING, DIM, LET, SWAP, GOTO, IF, THEN, ELSE, ON, FOR, TO,
      NEXT, STEP, WHILE, WEND, DEF, FN, GOSUB, RETURN, AND, OR, NOT, READ, DATA,
      RESTORE, INPUT, PRINT, LOCATE, INVERSE, INKEY, PLAY, BEEP, GRAPH, TEXT, DRAW,
      LINE, BOX, CIRCLE, ELLIPSE, OPEN, CLOSE, PUT, GET, LSET, RSET, CONT, POP, REM,
      CLEAR, WRITE, AS, POKE, CALL, CLS, FIELD, END, GE, LE, NEQ, TAB, SPC,
      SLEEP,
   };
};

class Lexer {
public:
   int tok;
   int32_t ival;
   double rval;
   std::string sval;

public:
   int getc();
   void skipSpace();
   int getToken() throw (int);

   Lexer(FILE *);

private:
   int c;
   FILE *fp;
   std::unordered_map<std::string, int> kwmap;

private:
   int peek();
   bool peek(int);
};

}

#endif //GVBASIC_LEXER_H
