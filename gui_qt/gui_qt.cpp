#include "gui_qt.h"
#include <QVBoxLayout>
#include <QMenu>
#include <QMenuBar>
#include <QApplication>
#include <QStatusBar>
#include <QMessageBox>
#include <QLabel>
#include <QWidget>
#include <QDesktopWidget>
#include <thread>
#include "screen.h"

using namespace std;
using namespace gvbsim;


GuiQt::GuiQt() {
   setWindowTitle(tr("GVBASIC Simulator"));
   
   QWidget *central = new QWidget(this);
   
   auto status = new QLabel(this);
   
   m_screen = new Screen(status);
   
   connect(m_screen, &Screen::stopped, this, &GuiQt::stop, Qt::QueuedConnection);
   
   loadMenu();
   
   statusBar()->addWidget(status);
   statusBar()->setSizeGripEnabled(false);
   status->setText(tr("haha"));
   status->setFrameStyle(QFrame::Box);
   
   QVBoxLayout *layout = new QVBoxLayout(central);
   layout->addWidget(m_screen, 0, 0);
   
   setCentralWidget(central);
   
   QRect sz = QApplication::desktop()->screenGeometry();
   move((sz.width() - width()) / 2, (sz.height() - height()) / 2);
}

void GuiQt::loadMenu() {
   QMenu *m = menuBar()->addMenu(tr("&File"));
   m_mnuOpen = m->addAction(tr("&Open"), this, &GuiQt::loadFile, QKeySequence::Open);
   m->addSeparator();
   m->addAction(tr("&Quit"), qApp, &QApplication::quit, QKeySequence(Qt::ALT, Qt::Key_F4));
   
   m = menuBar()->addMenu(tr("&Program"));
   m_mnuRun = m->addAction(tr("Run"), this, &GuiQt::run, QKeySequence(Qt::Key_F5));
   m_mnuStop = m->addAction(tr("&Stop"), this, &GuiQt::stop, QKeySequence(Qt::Key_F6));
   m->addSeparator();
   m->addAction(tr("&Capture Screen"), m_screen, &Screen::captureScreen, QKeySequence(Qt::Key_F9));
   m->addSeparator();
   m->addAction(tr("&Reload Config"), m_screen, &Screen::loadConfig);
   
   m = menuBar()->addMenu(tr("&Help"));
   m->addAction(tr("&Content"), this, &GuiQt::showHelp, QKeySequence(Qt::Key_F1));
   m->addSeparator();
   m->addAction(tr("&About"), this, &GuiQt::showAbout);
   m->addAction(tr("&About Qt"), qApp, &QApplication::aboutQt);
   
   m_mnuRun->setEnabled(false);
   m_mnuStop->setEnabled(false);
}


void GuiQt::loadFile() {
   if (m_screen->loadFile()) {
      m_mnuRun->setEnabled(true);
   }
}

void GuiQt::keyPressEvent(QKeyEvent *e) {
   m_screen->keyDown(e);
}

void GuiQt::keyReleaseEvent(QKeyEvent *e) {
   m_screen->keyUp(e);
}

void GuiQt::run() {
   switch (m_screen->run()) {
   case Screen::Result::Start:
      m_mnuStop->setEnabled(true);
      m_mnuOpen->setEnabled(false);
      m_mnuRun->setText(tr("Pause"));
      break;
   case Screen::Result::Resume:
      m_mnuStop->setEnabled(true);
      m_mnuRun->setText(tr("Pause"));
      break;
   case Screen::Result::Pause:
      m_mnuRun->setText(tr("Run"));
      break;
   }
}

void GuiQt::stop() {
   m_screen->stop();
   m_mnuRun->setText(tr("Run"));
   m_mnuOpen->setEnabled(true);
   m_mnuStop->setEnabled(false);
}

void GuiQt::showHelp() {
}

void GuiQt::showAbout() {
   QMessageBox::about(this,
                tr("About GVBASIC Simulator"),
                tr("Made by Plodsoft (Github: arucil)\n"
                   "Distributed under the MIT License"));
}

GuiQt::~GuiQt() {
}

