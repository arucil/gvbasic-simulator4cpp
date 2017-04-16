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
   k(SPC);
   k(SLEEP); k(PAINT); k(LOAD); k(FPUTC); k(FWRITE); k(FREAD); k(FSEEK);
   case Token::INKEY: return "INKEY$";
   case -1: return "EOF";
   case 10: return "EOL";
   case Token::GE: return ">=";
   case Token::LE: return "<=";
   case Token::NEQ: return "<>";
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

const unordered_map<string, int> Lexer::s_kwmap = {
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
   kw(PAINT), kw(LOAD), kw(FPUTC), kw(FWRITE), kw(FREAD), kw(FSEEK),
   { "INKEY$", Token::INKEY }
};

#undef kw

Lexer::Lexer(FILE *fp) : m_fp(fp), m_c(' ') { }

int Lexer::peek() {
   return m_c = (feof(m_fp) ? -1 : std::getc(m_fp));
}

bool Lexer::peek(int ch) {
   if (peek() != ch)
      return false;
   peek();
   return true;
}

int Lexer::getc() {
   int d = m_c;
   peek();
   return d;
}

void Lexer::skipSpace() {
   while (' ' == m_c)
      peek();
}

int Lexer::getToken() {
   while (' ' == m_c || 0xd == m_c)
      peek();

   switch (m_c) {
   case 0xa:
      peek();
      return 0xa;
   case '<':
      if (peek('>'))
         return Token::NEQ;
      else if ('=' == m_c) {
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
      while (peek() != '"' && m_c != 0xd && m_c != -1 && m_c != 0xa)
         sval += static_cast<char>(m_c);
      if ('"' == m_c)
         peek();
      return Token::STRING;
   }

   if (m_c >= '0' && m_c <= '9' || '.' == m_c) {
      sval.clear();
      while (m_c >= '0' && m_c <= '9') {
         sval += static_cast<char>(m_c);
         peek();
      }
      // 超过4位当做浮点数
      if (m_c != '.' && m_c != 'E' && m_c != 'e' && sval.size() < 5) {
         ival = std::atoi(sval.c_str());
         return Token::INT;
      }

      if ('.' == m_c) {
         do {
            sval += static_cast<char>(m_c);
         } while (peek() >= '0' && m_c <= '9');
         if (1 == sval.size())
            return '.';
      }

      if ('E' == m_c || 'e' == m_c) {
         sval += 'E';
         peek();
         if ('+' == m_c || '-' == m_c) {
            sval += static_cast<char>(m_c);
            peek();
         }
         while (m_c >= '0' && m_c <= '9') {
            sval += static_cast<char>(m_c);
            peek();
         }
      }

      const char *pp = sval.c_str();
      char *ppp;
      rval = std::strtod(pp, &ppp);
      if (ppp - pp != sval.size())
         throw NumberFormatError();
      return Token::REAL;
   }

   if (m_c >= 'A' && m_c <= 'Z' || m_c >= 'a' && m_c <= 'z') {
      sval.clear();
      do {
         sval += toupper(m_c);
      } while (peek() >= 'A' && m_c <= 'Z' || m_c >= '0' && m_c <= '9'
            || m_c >= 'a' && m_c <= 'z');
      if ('%' == m_c || '$' == m_c) {
         sval += static_cast<char>(m_c);
         peek();
      }
      auto i = s_kwmap.find(sval);
      if (s_kwmap.end() == i)
         return Token::ID;
      return i->second;
   }
   int t = m_c;
   peek();
   return t;
}