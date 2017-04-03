#include <iostream>
#include "../lex.h"
#include <cassert>

using namespace std;
using namespace gvbsim;

namespace {

void test(const char *fn, ostream &out) {
   FILE *fp = fopen(fn, "rb");
   assert(fp);
   Lexer l(fp);

   int tok;
   do {
      tok = l.getToken();
      switch (tok) {
      case Token::ID: out << "id:[" << l.sval << "]"; break;
      case Token::STRING: out << "str:\"" << l.sval << "\""; break;
      case Token::INT: out << "int:" << l.ival; break;
      case Token::REAL: out << "real:" << l.rval; break;
      default:
         if (tok > 32 && tok < 128)
            out << (char) tok;
         else
            out << Token::toString(tok);
      }
      out << endl;
   } while (tok != -1);
}

}

#if 1

int main() {
   test("test_case/lex_1.in", cout);
}

#endif
