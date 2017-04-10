#include "gui_qt.h"
#include <QVBoxLayout>
#include <QMenu>
#include <QMenuBar>
#include <QApplication>
#include <QStatusBar>
#include <QMessageBox>
#include <QLabel>
#include <QWidget>
#include <thread>
#include "screen.h"

using namespace std;
using namespace gvbsim;


GuiQt::GuiQt() {
   setWindowTitle(tr("GVBASIC Simulator"));
   
   QWidget *central = new QWidget(this);
   
   m_screen = new Screen(nullptr);
   
   loadMenu();
   
   m_status = new QLabel(this);
   statusBar()->addWidget(m_status);
   statusBar()->setSizeGripEnabled(false);
   m_status->setText(tr("haha"));
   m_status->setFrameStyle(QFrame::Box);
   
   QVBoxLayout *layout = new QVBoxLayout(central);
   layout->addWidget(m_screen, 0, 0);
   
   setCentralWidget(central);
}

void GuiQt::loadMenu() {
   QMenu *m = menuBar()->addMenu(tr("&File"));
   m_mnuOpen = m->addAction(tr("&Open"), this, &GuiQt::loadFile, QKeySequence::Open);
   m->addSeparator();
   m->addAction(tr("&Quit"), qApp, &QApplication::quit, QKeySequence(Qt::ALT, Qt::Key_F4));
   
   m = menuBar()->addMenu(tr("&Program"));
   m_mnuRun = m->addAction(tr("&Run"), m_screen, &Screen::start, QKeySequence(Qt::Key_F5));
   m_mnuStop = m->addAction(tr("&Stop"), m_screen, &Screen::stop, QKeySequence(Qt::Key_F6));
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
      m_status->setText(tr("Ready"));
   }
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

