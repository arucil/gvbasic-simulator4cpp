#include "gvb.h"
#include <sstream>
#include <cstdarg>
#include <cassert>
#include "error.h"
#include "lex.h"
#include "compile.h"

using namespace gvbsim;
using namespace std;


GVB::GVB(Device *dev) : m_device(dev), m_head(nullptr) {
   assert(dev);
}

GVB::~GVB() {
   clearStack();
   clearEnv();
}

void GVB::build(std::FILE *fp) {
   Compiler compiler(fp, m_nodeMan, m_dataMan);

   m_head = compiler.compile();
}

/* %c token
 * %s string
 * %S const char *
 * %t Value::Type
 * %i int
 * %f double
 * */
void GVB::error(int line, int label, const char *s, ...) {
   ostringstream sout;
   va_list a;

   sout.precision(9);
   va_start(a, s);
   while (*s) {
      if (*s == '%') {
         switch (*++s) {
         case '%':
            sout.put('%');
            break;
         case 'c': { //token
            int t = va_arg(a, int);
            if (t > 32 && t < 128)
               sout << static_cast<char>(t);
            else
               sout << Token::toString(t);
            break;
         }
         case 's': { //string
            string s = va_arg(a, string);
            if (s.length() > 30) {
               s.resize(30);
            }
            sout << s;
            break;
         }
         case 'S':
            sout << va_arg(a, const char *);
            break;
         case 't': //value type
            sout << Value::toString(va_arg(a, Value::Type));
            break;
         case 'i': //integer
            sout << va_arg(a, int);
            break;
         case 'f': //double
            sout << va_arg(a, double);
            break;
         default:
            s--;
         }
      } else
         sout.put(*s);
      s++;
   }
   va_end(a);

   throw Exception(line, label, sout.str());
}
