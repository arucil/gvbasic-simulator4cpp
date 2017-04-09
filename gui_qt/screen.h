#ifndef SCREEN_H
#define SCREEN_H

#include <QFrame>
#include <QImage>
#include <QString>
#include <cstdint>

class QPaintEvent;
namespace gvbsim {
   class Device;
}
class GuiQt;
class QTimer;


class Screen : public QFrame {
   Q_OBJECT
   
public:
   explicit Screen(GuiQt *parent, gvbsim::Device &);
   
public:
   void setScale(int);
   void setImage(std::uint8_t *, QRgb fg, QRgb bg);
   
   void setError(const QString &);
   void startTimer();
   void stopTimer();
   
protected:
   void paintEvent(QPaintEvent *event);
   
private slots:
   void blink();
   
private:
   QImage m_img;
   int m_scale;
   GuiQt *m_parent;
   gvbsim::Device &m_device;
   QString m_error;
   QTimer *m_timer;
};


#endif // SCREEN_H
