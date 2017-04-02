#include <cstdlib>
#include <cassert>
#include "lex.h"

using namespace std;
using namespace gvbsim;


#define k(s)   case s: return #s

const char *Token::toString(int tok) {
   switch (tok) {
   k(DIM); k(LET); k(SWAP); k(GOTO); k(IF); k(THEN); k(ELSE); k(ON); k(FOR);
   k(TO); k(NEXT); k(STEP); k(WHILE); k(WEND); k(DEF); k(FN); k(GOSUB);
   k(RETURN); k(NOT); k(AND); k(OR); k(READ); k(DATA); k(RESTORE); k(INPUT);
   k(PRINT); k(LOCATE); k(INVERSE); k(PLAY); k(BEEP); k(GRAPH); k(TEXT);
   k(DRAW); k(LINE); k(BOX); k(CIRCLE); k(ELLIPSE); k(OPEN); k(CLOSE);
   k(PUT); k(GET); k(LSET); k(RSET); k(CONT); k(POP); k(REM); k(CLEAR);
   k(WRITE); k(AS); k(POKE); k(CALL); k(CLS); k(FIELD); k(END); k(TAB);
   k(SPC); k(SLEEP);
   case Token::INKEY: return "INKEY$";
   case -1: return "EOF";
   case 10: return "EOL";
   case ID: return "id";
   case REAL: return "real";
   case INT: return "int";
   case STRING: return "string";
   default:
      assert(0);
   }
}

#undef k


#define kw(s)  { #s, Token::s }

Lexer::Lexer(FILE *fp) : fp(fp), c(' '),
                         kwmap {
      kw(DIM), kw(LET), kw(SWAP), kw(GOTO), kw(IF), kw(THEN),
      kw(ELSE), kw(ON), kw(FOR), kw(TO), kw(NEXT), kw(STEP),
      kw(WHILE), kw(WEND), kw(DEF), kw(FN), kw(GOSUB), kw(RETURN),
      kw(NOT), kw(AND), kw(OR), kw(READ), kw(DATA), kw(RESTORE),
      kw(INPUT), kw(PRINT), kw(LOCATE), kw(INVERSE),
      kw(PLAY), kw(BEEP), kw(GRAPH), kw(TEXT), kw(DRAW), kw(LINE),
      kw(BOX), kw(CIRCLE), kw(ELLIPSE), kw(OPEN), kw(CLOSE),
      kw(PUT), kw(GET), kw(LSET), kw(RSET), kw(CONT), kw(POP),
      kw(REM), kw(CLEAR), kw(WRITE), kw(AS), kw(POKE), kw(CALL),
      kw(CLS), kw(FIELD), kw(END), kw(TAB), kw(SPC), kw(SLEEP),
      { "INKEY$", Token::INKEY }
   } {
}

#undef kw

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

int Lexer::getToken() {
   while (' ' == c || 0xd == c)
      peek();

   switch (c) {
   case 0xa:
      peek();
      return 0xa;
   case '<':
      if (peek('>'))
         return Token::NEQ;
      else if ('=' == c) {
         peek();
         return Token::LE;
      }
      return '<';
   case '>':
      if (peek('='))
         return Token::GE;
      return '>';
   case '"':
      sval.clear();
      while (peek() != '"' && c != 0xd && c != -1 && c != 0xa)
         sval += static_cast<char>(c);
      if ('"' == c)
         peek();
      return Token::STRING;
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
         return Token::INT;
      }

      if ('.' == c) {
         do {
            sval += static_cast<char>(c);
         } while (peek() >= '0' && c <= '9');
         if (1 == sval.size())
            return '.';
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
      return Token::REAL;
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
         return Token::ID;
      return i->second;
   }
   int t = c;
   peek();
   return t;
}