#include "compile.h"
#include "gvb.h"

using namespace gvbsim;
using namespace std;

#define cerror(...)  GVB::error(this->line, this->label, __VA_ARGS__)

Compiler::Compiler(FILE *fp, NodeManager &nm)
      : l(fp), nodeMan(nm) {
}

void Compiler::peek() {
   try {
      tok = l.getToken();
   } catch (int) {
      cerror("Number format error");
   }
}

void Compiler::match(int t) {
   if (t != tok)
      cerror("Token error: [%c], expected: [%c]", tok, t);
   peek();
}
