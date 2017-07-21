#ifndef SCREEN_H
#define SCREEN_H

#include <QWidget>
#include <QImage>
#include <QFileDialog>
#include <QString>
#include <QBasicTimer>
#include <thread>
#include <atomic>
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
class QTableWidget;
class QTableWidgetItem;


class Screen : public QWidget, public gvbsim::IGui {
   Q_OBJECT
   
   enum class State {
      Initial, Ready, Running, Paused, Quit
   };
   
public:
   enum class Result {
      Start, Resume, Pause, Ready
   };
   
public:
   explicit Screen(QLabel *status, QLabel *im, QTableWidget *table);
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
   bool loadFile(bool bReload = false);
   Result run();
   void stop();
   
   void keyDown(QKeyEvent *);
   void keyUp(QKeyEvent *);

   QString getOpenFile() const {
       return m_openFile;
   }
   
private:
   void initFileDlg();
   void setError(const QString &);
   void setScale(int);
   void setImage(std::uint8_t *, QRgb fg, QRgb bg);
   
   void threadRun();
   
   void clearCursor();
   
   void loadVarList();
   void setItemText(QTableWidgetItem *, gvbsim::Value::Type, gvbsim::GVB::Single *);
   
protected:
   void paintEvent(QPaintEvent *);
   void resizeEvent(QResizeEvent *);
   void timerEvent(QTimerEvent *);
   
public slots:
   void captureScreen();
   void loadConfig();
   
private slots:
   void blink();
   void varDoubleClicked(int row, int col);
   
signals:
   void stopped();
   void showInputMethod();
   void hideInputMethod();
   
private:
   QImage m_img;
   int m_scale;
   QString m_error;
   QString m_openFile;
   QBasicTimer m_timerUpdate;
   QTimer *m_timerBlink;
   QFileDialog m_fileDlg;
   int m_sleepFactor;
   QLabel *m_status;
   QLabel *m_im;
   QTableWidget *m_table;
   std::thread m_thread;
   std::mutex m_mutState;
   std::condition_variable m_cv;
   std::atomic_int m_needUpdate;
   State m_state = State::Initial;
   
   gvbsim::GVB m_gvb;
   
private:
   static const std::unordered_map<int, int> s_wqxKeyMap; // pc key code -> wqx
};


#endif // SCREEN_H
