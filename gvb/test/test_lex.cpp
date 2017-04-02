#include <iostream>
#include "../lex.h"
#include <cassert>

using namespace std;
using namespace gvbsim;

namespace {

#define k(s) case Token::s: return #s

string tokenstr(int tok) {
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
   default:
      assert(0);
   }
}

void test(const char *fn, ostream &out) {
   FILE *fp = fopen(fn, "rb");
   assert(fp);
   Lexer l(fp);

   int tok;
   do {
      tok = l.getToken();
      switch (tok) {
      case Token::GE: out << ">="; break;
      case Token::LE: out << "<="; break;
      case Token::NEQ: out << "<>"; break;
      case Token::ID: out << "id:[" << l.sval << "]"; break;
      case Token::STRING: out << "str:\"" << l.sval << "\""; break;
      case Token::INT: out << "int:" << l.ival; break;
      case Token::REAL: out << "real:" << l.rval; break;
      default:
         if (tok > 32 && tok < 128)
            out << (char) tok;
         else
            out << tokenstr(tok);
      }
      out << endl;
   } while (tok != -1);
}

}

#if 1

int main() {
   test("test_case/lex-1.in", cout);
}

#endif
