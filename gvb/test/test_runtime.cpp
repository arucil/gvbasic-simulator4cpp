#include <cstdio>
#include "../gvb.h"
#include "../error.h"
#include <cassert>
#include <iostream>

using namespace std;
using namespace gvbsim;

namespace {

void test(FILE *fp) {
   GVB gvb;

   try {
      gvb.build(fp);

      gvb.execute(0);
   } catch (Exception &e) {
      cout << e.label << "(line:" << e.line << "): " << e.msg << endl;
      throw;
   }

}

}


int main() {
   FILE *fp = fopen("test_case/runtime_4.in", "rb");
   assert(fp);
   test(fp);
}
