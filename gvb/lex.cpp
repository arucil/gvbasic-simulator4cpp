#include <cstdlib>
#include "lex.h"

using namespace std;
using namespace gvbsim;

#define __kw(s)  { #s, Token::s }

Lexer::Lexer(FILE *fp) : fp(fp), tok(0), c(' '), kwmap {
      __kw(DIM), __kw(LET), __kw(SWAP), __kw(GOTO), __kw(IF), __kw(THEN),
      __kw(ELSE), __kw(ON), __kw(FOR), __kw(TO), __kw(NEXT), __kw(STEP),
      __kw(WHILE), __kw(WEND), __kw(DEF), __kw(FN), __kw(GOSUB), __kw(RETURN),
      __kw(NOT), __kw(AND), __kw(OR), __kw(READ), __kw(DATA), __kw(RESTORE),
      __kw(INPUT), __kw(PRINT), __kw(LOCATE), __kw(INVERSE),
      __kw(PLAY), __kw(BEEP), __kw(GRAPH), __kw(TEXT), __kw(DRAW), __kw(LINE),
      __kw(BOX), __kw(CIRCLE), __kw(ELLIPSE), __kw(OPEN), __kw(CLOSE),
      __kw(PUT), __kw(GET), __kw(LSET), __kw(RSET), __kw(CONT), __kw(POP),
      __kw(REM), __kw(CLEAR), __kw(WRITE), __kw(AS), __kw(POKE), __kw(CALL),
      __kw(CLS), __kw(FIELD), __kw(END), __kw(TAB), __kw(SPC), __kw(SLEEP),
      { "INKEY$", Token::INKEY }
   } {
}

int Lexer::peek() {
   return c = (feof(fp) ? -1 : std::getc(fp));
}

bool Lexer::peek(int ch) {
   if (peek() != ch)
      return false;
   peek();
   return true;
}

int Lexer::getc() {
   int d = c;
   peek();
   return d;
}

void Lexer::skipSpace() {
   while (' ' == c)
      peek();
}

int Lexer::getToken() throw (int) {
   while (' ' == c || 0xd == c)
      peek();

   switch (c) {
   case 0xa:
      peek();
      return tok = 0xa;
   case '<':
      if (peek('>'))
         return tok = Token::NEQ;
      else if ('=' == c) {
         peek();
         return tok = Token::LE;
      }
      return tok = '<';
   case '>':
      if (peek('='))
         return tok = Token::GE;
      return tok = '>';
   case '"':
      sval.clear();
      while (peek() != '"' && c != 0xd && c != -1 && c != 0xa)
         sval += static_cast<char>(c);
      if ('"' == c)
         peek();
      return tok = Token::STRING;
   }

   if (c >= '0' && c <= '9' || '.' == c) {
      sval.clear();
      while (c >= '0' && c <= '9') {
         sval += static_cast<char>(c);
         peek();
      }
      // 超过4位当做浮点数
      if (c != '.' && c != 'E' && c != 'e' && sval.size() < 5) {
         ival = std::atoi(sval.c_str());
         return tok = Token::INT;
      }

      if ('.' == c) {
         do {
            sval += static_cast<char>(c);
         } while (peek() >= '0' && c <= '9');
         if (1 == sval.size())
            return tok = '.';
      }

      if ('E' == c || 'e' == c) {
         sval += 'E';
         peek();
         if ('+' == c || '-' == c) {
            sval += static_cast<char>(c);
            peek();
         }
         while (c >= '0' && c <= '9') {
            sval += static_cast<char>(c);
            peek();
         }
      }

      const char *pp = sval.c_str();
      char *ppp;
      rval = std::strtod(pp, &ppp);
      if (ppp - pp != sval.size())
         throw 1;
      return tok = Token::REAL;
   }

   if (c >= 'A' && c <= 'Z' || c >= 'a' && c <= 'z') {
      sval.clear();
      do {
         sval += toupper(c);
      } while (peek() >= 'A' && c <= 'Z' || c >= '0' && c <= '9'
            || c >= 'a' && c <= 'z');
      if ('%' == c || '$' == c) {
         sval += static_cast<char>(c);
         peek();
      }
      auto i = kwmap.find(sval);
      if (kwmap.end() == i)
         return tok = Token::ID;
      return tok = i->second;
   }
   tok = c;
   peek();
   return tok;
}