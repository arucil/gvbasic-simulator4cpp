#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include "../gvb/igui.h"
#include "../gvb/gvb.h"


class Screen;


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
   QFileDialog m_fileDlg;
   
   gvbsim::GVB m_gvb;
};


#endif // MAINWINDOW_H
