#ifndef SCREEN_H
#define SCREEN_H

#include <QWidget>
#include <QImage>
#include <QFileDialog>
#include <QString>
#include "../gvb/gvb.h"
#include "../gvb/igui.h"

class QPaintEvent;
class QResizeEvent;
class GuiQt;
class QTimer;


class Screen : public QWidget, public gvbsim::IGui {
   Q_OBJECT
   
public:
   explicit Screen(QWidget *parent);
   
public:
   void update();
   void update(int x1, int y1, int x2, int y2);
   void sleep(int ticks);
   
public:
   bool loadFile();
   
private:
   void initFileDlg();
   void startTimer();
   void stopTimer();
   void setError(const QString &);
   void setScale(int);
   void setImage(std::uint8_t *, QRgb fg, QRgb bg);
   
protected:
   void paintEvent(QPaintEvent *);
   void resizeEvent(QResizeEvent *);
   
private slots:
   void blink();
   
public slots:
   void start();
   void stop();
   void captureScreen();
   void loadConfig();
   
private:
   QImage m_img;
   int m_scale;
   QString m_error;
   QTimer *m_timer;
   QFileDialog m_fileDlg;
   int m_sleepFactor;
   
   gvbsim::GVB m_gvb;
};


#endif // SCREEN_H
