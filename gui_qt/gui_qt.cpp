#include "gui_qt.h"
#include <QThread>
#include <cstdlib>
#include <QBoxLayout>
#include <QMenu>
#include <QMenuBar>
#include <QApplication>
#include <QStatusBar>
#include <QMessageBox>
#include "screen.h"
#include "readconfig.h"

using namespace std;
using namespace gvbsim;


GuiQt::GuiQt() : m_fileDlg(this) {
   Device::loadData();
   
   setWindowTitle(tr("GVBASIC Simulator"));
   
   auto l = new QVBoxLayout(this);
   m_screen = new Screen(this);
   l->addWidget(m_screen);
   l->setAlignment(m_screen, Qt::AlignHCenter);
   loadMenu();
   
   loadConfig();
   
   setFixedSize(350, 166 + menuBar()->geometry().height() + statusBar()->geometry().height());
   
   initFileDlg();
   
   statusBar()->showMessage(tr("ha"));
}

void GuiQt::loadConfig() {
   gvbsim::ConfigReader cr;
   cr.load("config.ini");
   
   auto &dev1 = cr.section("Device");
   
   m_gvb.getDevice().setGraphAddr(static_cast<uint16_t>(
         strtol(dev1["graphbuffer"].c_str(), nullptr, 0)));
   m_gvb.getDevice().setTextAddr(static_cast<uint16_t>(
        strtol(dev1["textbuffer"].c_str(), nullptr, 0)));
   m_gvb.getDevice().setKeyAddr(static_cast<uint16_t>(
        strtol(dev1["keybuffer"].c_str(), nullptr, 0)));
   m_gvb.getDevice().setKeyMapAddr(static_cast<uint16_t>(
        strtol(dev1["keybuffer"].c_str(), nullptr, 0)));
   
   auto &gui1 = cr.section("Gui");
   
   m_screen->setScale(atoi(gui1["scale"].c_str()));
   m_screen->setImage(m_gvb.getDevice().getGraphBuffer(),
                      strtol(gui1["fgcolor"].c_str(), nullptr, 0),
                      strtol(gui1["bgcolor"].c_str(), nullptr, 0));
}

void GuiQt::loadMenu() {
   QMenu *m = menuBar()->addMenu(tr("&File"));
   m->addAction(tr("&Load"), this, &GuiQt::loadFile, QKeySequence(Qt::CTRL, Qt::Key_O));
   m->addSeparator();
   m->addAction(tr("&Quit"), qApp, &QApplication::quit, QKeySequence(Qt::ALT, Qt::Key_F4));
   
   m = menuBar()->addMenu(tr("&Program"));
   m->addAction(tr("&Run"), this, &GuiQt::start, QKeySequence(Qt::Key_F5));
   m->addAction(tr("&Stop"), this, &GuiQt::stop, QKeySequence(Qt::Key_F6));
   m->addSeparator();
   m->addAction(tr("&Capture Screen"), this, &GuiQt::captureScreen, QKeySequence(Qt::Key_F9));
   
   m = menuBar()->addMenu(tr("&Help"));
   m->addAction(tr("&Content"), this, &GuiQt::showHelp, QKeySequence(Qt::Key_F1));
   m->addSeparator();
   m->addAction(tr("&About"), this, &GuiQt::showAbout);
   m->addAction(tr("&About Qt"), qApp, &QApplication::aboutQt);
}

void GuiQt::initFileDlg() {
   m_fileDlg.setFileMode(QFileDialog::ExistingFile);
   m_fileDlg.setNameFilter(tr("Text File (*.txt)"));
}

void GuiQt::loadFile() {
   if (m_fileDlg.exec()) {
   }
}

void GuiQt::start() {
}

void GuiQt::stop() {
}

void GuiQt::captureScreen() {
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

void GuiQt::update() {
   QWidget::update();
}

void GuiQt::update(int x1, int y1, int x2, int y2) {
   QWidget::update(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}

void GuiQt::sleep(int ticks) {
   if (ticks > 0) {
      QThread::usleep(ticks);
   }
}
