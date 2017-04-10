#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>


class Screen;
class QLabel;
class QAction;


class GuiQt : public QMainWindow {
   Q_OBJECT
   
public:
   GuiQt();
   ~GuiQt();
   
   
private:
   
   void loadMenu();
   
   
private slots:
   void loadFile();
   void showHelp();
   void showAbout();
   
private:
   Screen *m_screen;
   QLabel *m_status;
   QAction *m_mnuOpen, *m_mnuRun, *m_mnuStop;
   
};


#endif // MAINWINDOW_H
