#ifndef SCREEN_H
#define SCREEN_H

#include <QWidget>
#include <QImage>
#include <QFileDialog>
#include <QString>
#include <QBasicTimer>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include "../gvb/gvb.h"
#include "../gvb/igui.h"

class QPaintEvent;
class QTimerEvent;
class QResizeEvent;
class QKeyEvent;
class GuiQt;
class QLabel;
class QTimer;


class Screen : public QWidget, public gvbsim::IGui {
   Q_OBJECT
   
   enum class State {
      Initial, Ready, Running, Paused, Quit
   };
   
public:
   enum class Result {
      Start, Resume, Pause
   };
   
public:
   explicit Screen(QLabel *status, QLabel *im);
   ~Screen();
   
public:
   void update() override;
   void update(int x1, int y1, int x2, int y2) override;
   void sleep(int ticks) override;
   bool isStopped() override;
   
   void beginInput() override;
   void switchIM(InputMethod) override;
   void endInput() override;
   
public:
   bool loadFile();
   Result run();
   void stop();
   
   void keyDown(QKeyEvent *);
   void keyUp(QKeyEvent *);
   
private:
   void initFileDlg();
   void setError(const QString &);
   void setScale(int);
   void setImage(std::uint8_t *, QRgb fg, QRgb bg);
   
   void threadRun();
   
   void clearCursor();
   
protected:
   void paintEvent(QPaintEvent *);
   void resizeEvent(QResizeEvent *);
   void timerEvent(QTimerEvent *);
   
public slots:
   void captureScreen();
   void loadConfig();
   
signals:
   void stopped();
   
private:
   QImage m_img;
   int m_scale;
   QString m_error;
   QBasicTimer m_timer;
   QFileDialog m_fileDlg;
   int m_sleepFactor;
   QLabel *m_status;
   QLabel *m_inputM;
   std::thread m_thread;
   std::mutex m_mutState;
   std::condition_variable m_cv;
   State m_state = State::Initial;
   
   gvbsim::GVB m_gvb;
   
private:
   static const std::unordered_map<int, int> s_wqxKeyMap; // pc key code -> wqx
};


#endif // SCREEN_H
