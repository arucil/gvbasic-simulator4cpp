#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include "../gvb/igui.h"
#include "../gvb/gvb.h"


class Screen;
class QLabel;
class QAction;
namespace std {
   class thread;
}


class GuiQt : public QMainWindow, public gvbsim::IGui {
   Q_OBJECT
   
public:
   GuiQt();
   ~GuiQt();
   
public:
   void update();
   void update(int x1, int y1, int x2, int y2);
   void sleep(int ticks);
   
private:
   void loadConfig();
   
   void loadMenu();
   
   void initFileDlg();
   
private slots:
   void loadFile();
   void start();
   void stop();
   void captureScreen();
   void showHelp();
   void showAbout();
   
private:
   Screen *m_screen;
   QLabel *m_status;
   QAction *m_mnuOpen, *m_mnuRun, *m_mnuStop;
   QFileDialog m_fileDlg;
   bool m_running;
   std::thread *m_thread;
   
   gvbsim::GVB m_gvb;
   
};


#endif // MAINWINDOW_H
