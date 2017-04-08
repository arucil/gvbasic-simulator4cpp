#include "gvb.h"
#include <sstream>
#include <cstdarg>
#include <cstring>
#include "error.h"
#include "lex.h"
#include "compile.h"

using namespace gvbsim;
using namespace std;


GVB::GVB() : m_head(nullptr) {
}

GVB::~GVB() {
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
 * %m file mode
 * */
void GVB::error(int line, int label, const char *s, ...) {
   ostringstream sout;
   va_list a;

   // %.9g
   sout.precision(9);
   sout.unsetf(ios_base::floatfield);

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
            else if (t > Token::TOKEN_FIRST && t < Token::TOKEN_LAST)
               sout << Token::toString(t);
            else
               sout << t;
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
         case 'm':
            sout << File::toString(va_arg(a, File::Mode));
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

string &GVB::removeAllOf(std::string &s, const char *c, size_t n) {
   bool tab[128];
   memset(tab, 0, sizeof tab);

   while (n-- > 0)
      tab[*c++] = true;

   size_t j = 0;
   for (size_t i = 0; i < s.size(); ++i) {
      if (!tab[s[i]])
         s[j++] = s[i];
   }
   s.resize(j);
   return s;
}

double GVB::str2d(const std::string &s) {
   auto p = s.data();
   while (' ' == *p)
      ++p;
   if ('-' == *p || '+' == *p)
      ++p;
   if (!isdigit(*p) && '.' != *p)
      return 0.;

   return strtod(s.data(), nullptr);
}