#ifndef GVBASIC_LEX_H
#define GVBASIC_LEX_H

#include <string>
#include <cstdio>
#include <unordered_map>

namespace gvbsim {

struct Token {
   enum {
      TOKEN_FIRST = 256,
      ID, INT, REAL, STRING, DIM, LET, SWAP, GOTO, IF, THEN, ELSE, ON, FOR, TO,
      NEXT, STEP, WHILE, WEND, DEF, FN, GOSUB, RETURN, AND, OR, NOT, READ, DATA,
      RESTORE, INPUT, PRINT, LOCATE, INVERSE, INKEY, PLAY, BEEP, GRAPH, TEXT, DRAW,
      LINE, BOX, CIRCLE, ELLIPSE, OPEN, CLOSE, PUT, GET, LSET, RSET, CONT, POP, REM,
      CLEAR, WRITE, AS, POKE, CALL, CLS, FIELD, END, GE, LE, NEQ, TAB, SPC,
      SLEEP, PAINT,
      TOKEN_LAST
   };

   static const char *toString(int tok);
};

class Lexer {
public:
   typedef int NumberFormatError;

public:
   int32_t ival;
   double rval;
   std::string sval;

public:
   int getc();
   void skipSpace();
   int getToken();

   Lexer(FILE *);

private:
   int m_c;
   FILE *m_fp;

   static const std::unordered_map<std::string, int> s_kwmap;

private:
   int peek();
   bool peek(int);
};

}

#endif //GVBASIC_LEXER_H
