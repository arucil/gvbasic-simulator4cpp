#include "screen.h"
#include <QPainter>
#include <QPaintEvent>
#include <QTimer>
#include "../gvb/device.h"
#include "gui_qt.h"

using namespace gvbsim;


Screen::Screen(GuiQt *parent, Device &device)
      : QFrame(parent), m_parent(parent), m_device(device) {
   
   m_timer = new QTimer(this);
   connect(timer, &QTimer::timeout, this, &Screen::blink);
}

void Screen::setScale(int scale) {
   if (scale < 1)
      scale = 2;
   m_scale = scale;
   resize(scale * 160, scale * 80);
}

void Screen::setError(const QString &s) {
   m_error = s;
   update();
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

void Screen::blink() {
   if (m_device.cursorEnabled()) {
      m_parent->flipCursor();
      update();
   }
}

void Screen::paintEvent(QPaintEvent *e) {
   QPainter painter(this);
   painter.scale(m_scale, m_scale);
   painter.drawImage(m_img.rect(), m_img);
   
   if (m_parent->m_displayCursor) { // display cursor
      int cx = m_device.getX() << 3,
            cy = m_device.getY() << 4,
            w = 8;
      
      switch (m_device.getPosInfo()) {
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