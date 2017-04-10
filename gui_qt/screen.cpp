#include "screen.h"
#include <QPainter>
#include <QPaintEvent>
#include <QTimer>
#include <QStringList>
#include <QThread>
#include <cstdlib>
#include <cstdio>
#include <mutex>
#include "../gvb/device.h"
#include "../gvb/error.h"
#include "readconfig.h"

using namespace gvbsim;
using namespace std;


Screen::Screen(QWidget *parent)
      : QWidget(parent), m_fileDlg(this) {
   m_timer = new QTimer(this);
   connect(m_timer, &QTimer::timeout, this, &Screen::blink);
   
   loadConfig();
   initFileDlg();
}

void Screen::loadConfig() {
   gvbsim::ConfigReader cr;
   cr.load("config.ini");
   
   auto &dev1 = cr.section("Device");
   
   m_gvb.device().setGraphAddr(static_cast<uint16_t>(
         strtol(dev1["graphbuffer"].c_str(), nullptr, 0)));
   m_gvb.device().setTextAddr(static_cast<uint16_t>(
        strtol(dev1["textbuffer"].c_str(), nullptr, 0)));
   m_gvb.device().setKeyAddr(static_cast<uint16_t>(
        strtol(dev1["keybuffer"].c_str(), nullptr, 0)));
   m_gvb.device().setKeyMapAddr(static_cast<uint16_t>(
        strtol(dev1["keybuffer"].c_str(), nullptr, 0)));
   
   auto &gui1 = cr.section("Gui");
   
   setScale(atoi(gui1["scale"].c_str()));
   setImage(m_gvb.device().getGraphBuffer(),
            strtol(gui1["fgcolor"].c_str(), nullptr, 0),
            strtol(gui1["bgcolor"].c_str(), nullptr, 0));
   
   m_sleepFactor = atoi(gui1["sleepfactor"].c_str());
}

void Screen::setScale(int scale) {
   if (scale < 1)
      scale = 2;
   m_scale = scale;
   setFixedSize(scale * 160, scale * 80);
}

void Screen::setError(const QString &s) {
   m_error = s;
   QWidget::update();
}

void Screen::startTimer() {
   m_timer->start(500);
}

void Screen::stopTimer() {
   m_timer->stop();
}

void Screen::setImage(uint8_t *data, QRgb fg, QRgb bg) {
   m_img = QImage(data, 160, 80, QImage::Format_MonoLSB);
   m_img.setColor(0, bg | 0xff00'0000);
   m_img.setColor(1, fg | 0xff00'0000);
}

void Screen::initFileDlg() {
   m_fileDlg.setFileMode(QFileDialog::ExistingFile);
   m_fileDlg.setNameFilter(tr("Text File (*.txt)"));
}

bool Screen::loadFile() {
   if (m_fileDlg.exec()) {
      FILE *fp = fopen(m_fileDlg.selectedFiles().at(0).toStdString().c_str(), "rb");
      if (nullptr == fp) {
         setError(tr("File open error"));
         return false;
      }
      
      try {
         m_gvb.build(fp);
      } catch (Exception &e) {
         setError(QStringLiteral("%1(line:%2): %3").arg(e.label).arg(e.line).arg(QString::fromStdString(e.msg)));
         return false;
      }
      
      fclose(fp);
      setError(QString());
      return true;
   }
   return false;
}

void Screen::start() {
}

void Screen::stop() {
}

void Screen::captureScreen() {
}

void Screen::blink() {
   if (m_gvb.device().cursorEnabled()) {
      flipCursor();
      QWidget::update();
   }
}

void Screen::paintEvent(QPaintEvent *e) {
   QPainter painter(this);
   painter.scale(m_scale, m_scale);
   {
      lock_guard<mutex> lock(m_gvb.device().getGraphMutex());
      painter.drawImage(m_img.rect(), m_img);
   }
   
   if (m_displayCursor) { // display cursor
      int cx = m_gvb.device().getX() << 3,
            cy = m_gvb.device().getY() << 4,
            w = 8;
      
      switch (m_gvb.device().getPosInfo()) {
      case Device::CursorPosInfo::GB1st:
         w <<= 1;
         break;
      case Device::CursorPosInfo::GB2nd:
         cx -= 8;
         break;
      default:
         break;
      }
      
      painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
      painter.fillRect(QRect(cx, cy, w, 16),
                       QColor(m_img.color(0) ^ m_img.color(1) | 0xff00'0000));
   }
   
   if (m_error.size()) {
      painter.scale(1. / m_scale, 1. / m_scale);
      painter.setPen(QColor(Qt::red));
      painter.drawText(0, 0, 160 * m_scale, 80 * m_scale, Qt::TextWordWrap, m_error);
   }
}

void Screen::resizeEvent(QResizeEvent *) {
   qobject_cast<QWidget *>(parent())->adjustSize();
   window()->adjustSize();
}

void Screen::update() {
   QWidget::update();
}

void Screen::update(int x1, int y1, int x2, int y2) {
   QWidget::update(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}

void Screen::sleep(int ticks) {
   if (ticks > 0) {
      QThread::usleep(ticks);
   }
}