#include "../compile.h"
#include "../data_man.h"
#include "../node_man.h"
#include "../error.h"
#include "debug.h"
#include <iostream>
#include <cassert>

using namespace std;
using namespace gvbsim;

namespace {

void test(FILE *fp, ostream &out) {
   try {
      NodeManager nm;
      DataManager dm;
      Compiler compiler(fp, nm, dm);

      printTree(compiler.compile(), cout);
   } catch (const Exception &e) {
      out << e.label << "(line:" << e.line << ") " << e.msg;
       // throw;
   }
}

}

#if 1

int main() {
   FILE *fp = fopen("test_case/CASTLE.in", "rb");
   assert(fp);
   test(fp, cout);
}

#endif
