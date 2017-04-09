#ifndef SCREEN_H
#define SCREEN_H

#include <QWidget>
#include <QImage>
#include <cstdint>

class QPaintEvent;
namespace gvbsim {
   class Device;
}


class Screen : public QWidget {
   Q_OBJECT
   
public:
   explicit Screen(QWidget *parent);
   
public:
   void setScale(int);
   void setImage(std::uint8_t *, QRgb fg, QRgb bg);
   
protected:
   void paintEvent(QPaintEvent *event);
   
private:
   QImage m_img;
   int m_scale;
   gvbsim::Device *m_device;
};


#endif // SCREEN_H
