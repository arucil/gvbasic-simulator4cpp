#include "screen.h"
#include <QPainter>
#include <QPaintEvent>

using namespace gvbsim;


Screen::Screen(QWidget *parent) : QWidget(parent) {
}

void Screen::setScale(int scale) {
   if (scale < 1)
      scale = 2;
   m_scale = scale;
   setGeometry(3, 3, scale * 160, scale * 80);
}

void Screen::setImage(uint8_t *data, QRgb fg, QRgb bg) {
   m_img = QImage(data, 160, 80, QImage::Format_MonoLSB);
   m_img.setColor(0, bg | 0xff00'0000);
   m_img.setColor(1, fg | 0xff00'0000);
}

void Screen::paintEvent(QPaintEvent *e) {
   QPainter painter(this);
   painter.scale(m_scale, m_scale);
   painter.drawImage(m_img.rect(), m_img);
}