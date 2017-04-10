#include <cstdio>
#include "../gvb.h"
#include "../error.h"
#include "../igui.h"
#include <cassert>
#include <iostream>

using namespace std;
using namespace gvbsim;

namespace {

struct GuiStub : IGui {
   void update() {}
   void update(int, int, int, int) {}
   void sleep(int) {}
};

void test(FILE *fp) {
   GVB gvb;
   GuiStub gui;

   gvb.device().setGui(&gui);
   gvb.device().setGraphAddr(0);
   gvb.device().setKeyAddr(0);
   gvb.device().setKeyMapAddr(0);
   gvb.device().setTextAddr(0);

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
