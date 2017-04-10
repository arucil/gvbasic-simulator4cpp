#include "gui_qt.h"
#include <QApplication>
#include "../gvb/device.h"

using namespace gvbsim;


int main(int argc, char *argv[]) {
   QApplication a(argc, argv);
   
   Device::loadData();
   
   GuiQt w;
   w.show();
   
   return a.exec();
}
