#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>


class Screen;
class QLabel;
class QAction;
class QKeyEvent;


class GuiQt : public QMainWindow {
   Q_OBJECT
   
public:
   GuiQt();
   ~GuiQt();
   
protected:
   void keyPressEvent(QKeyEvent *);
   void keyReleaseEvent(QKeyEvent *);
   
private:
   void loadMenu();
   
   
private slots:
   void loadFile();
   void run();
   void stop();
   void showHelp();
   void showAbout();
   
private:
   Screen *m_screen;
   QAction *m_mnuOpen, *m_mnuRun, *m_mnuStop;
   
};


#endif // MAINWINDOW_H
