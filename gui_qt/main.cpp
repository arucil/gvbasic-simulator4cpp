#include "gui_qt.h"
#include <QApplication>
#include "readconfig.h"

using namespace gvbsim;


int main(int argc, char *argv[]) {
   QApplication a(argc, argv);
   
   GuiQt w;
   w.show();
   
   return a.exec();
}
