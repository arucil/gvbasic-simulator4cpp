#include "screen.h"
#include <QPainter>
#include <QPaintEvent>
#include <QStringList>
#include <QThread>
#include <QLabel>
#include <QTimer>
#include <QDateTime>
#include <QTableWidget>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <mutex>
#include "../gvb/device.h"
#include "../gvb/error.h"
#include "readconfig.h"

using namespace gvbsim;
using namespace std;


#define k(s,r)  { Qt::Key_ ## s, r }

const unordered_map<int, int> Screen::s_wqxKeyMap {
   k(Escape, 27), k(F1, 28), k(F2, 29), k(F3, 30), k(F4, 31), k(Q, 'q'),
   k(W, 'w'), k(E, 'e'), k(R, 'r'), k(T, 't'), k(Y, 'y'), k(U, 'u'),
   k(I, 'i'), k(O, 'o'), k(P, 'p'), k(A, 'a'), k(S, 's'), k(D, 'd'),
   k(F, 'f'), k(G, 'g'), k(H, 'h'), k(J, 'j'), k(K, 'k'), k(L, 'l'),
   k(Return, 13), k(Z, 'z'), k(X, 'x'), k(C, 'c'), k(V, 'v'), k(B, 'b'),
   k(N, 'n'), k(M, 'm'), k(PageUp, 19), k(PageDown, 14), k(Up, 20),
   k(Down, 21), k(Left, 23), k(Right, 22), k(QuoteLeft, 25), k(Shift, 26),
   k(Control, 18), k(0, '0'), k(Period, '.'), k(Space, ' '),
   k(1, 'b'), k(2, 'n'), k(3, 'm'), k(4, 'g'), k(5, 'h'), k(6, 'j'),
   k(7, 't'), k(8, 'y'), k(9, 'u'),
   k(Backspace, 29)
};

#undef k


Screen::Screen(QLabel *status, QLabel *im, QTableWidget *table)
      : m_status(status),
        m_im(im),
        m_table(table),
        m_fileDlg(this) {
   loadConfig();
   initFileDlg();
   
   m_gvb.device().setGui(this);
   
   connect(this, &Screen::stopped, this, &Screen::stop, Qt::QueuedConnection);
   connect(this, &Screen::showInputMethod, m_im, &QLabel::show, Qt::QueuedConnection);
   connect(this, &Screen::hideInputMethod, m_im, &QLabel::hide, Qt::QueuedConnection);
   
   m_timerBlink = new QTimer(this);
   connect(m_timerBlink, &QTimer::timeout, this, &Screen::blink);
   
   m_timerUpdate.start(20, this); // 50 hz
   
   m_thread = std::thread(&Screen::threadRun, this);
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
        strtol(dev1["keymap"].c_str(), nullptr, 0)));
   
   auto &gui1 = cr.section("Gui");
   
   setScale(atoi(gui1["scale"].c_str()));
   setImage(m_gvb.device().getGraphBuffer(),
            strtol(gui1["fgcolor"].c_str(), nullptr, 0),
            strtol(gui1["bgcolor"].c_str(), nullptr, 0));
   
   m_table->setFixedHeight(100 * m_scale);
   
   m_sleepFactor = atoi(gui1["sleepfactor"].c_str());
   
   unordered_map<int, int> keyMap;
   auto &keys = cr.section("KeyMap");
   
   for (auto &i : keys) {
      keyMap.insert(make_pair(atoi(i.first.c_str()),
                              strtol(i.second.c_str(), nullptr, 16)));
   }
   m_gvb.device().setKeyMap(keyMap);
}

void Screen::setScale(int scale) {
   if (scale < 1)
      scale = 2;
   m_scale = scale;
   setFixedSize(scale * 160, scale * 80);
}

void Screen::setError(const QString &s) {
   m_error = s;
   update();
}

void Screen::setImage(uint8_t *data, QRgb fg, QRgb bg) {
   m_img = QImage(data, 160, 80, QImage::Format_Mono);
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
      m_status->setText(tr("Ready"));
      {
         lock_guard<mutex> lock(m_mutState);
         m_state = State::Ready;
      }
      return true;
   }
   return false;
}

void Screen::threadRun() {
   while (true) {
      {
         unique_lock<mutex> lock(m_mutState);
         // waiting for start
         while (State::Initial == m_state || State::Ready == m_state)
            m_cv.wait(lock);
      }
      
      // init
      m_gvb.reset();
      m_displayCursor = false;
      m_error.clear();
      
      try {
         do {
            unique_lock<mutex> lock(m_mutState);
            
            if (State::Paused == m_state) {
               while (State::Paused == m_state)
                  m_cv.wait(lock);
            }
            
            if (State::Ready == m_state) {
               goto L1;
            }
            
            if (State::Quit == m_state)
               return;
         } while (m_gvb.step());
      } catch (Exception &e) {
         setError(QStringLiteral("%1(line:%2): %3").arg(e.label).arg(e.line).arg(QString::fromStdString(e.msg)));
      }
      
      emit stopped();
      
L1:
      // prevent from skipping the wait for start
      while (State::Ready != m_state) {
         QThread::msleep(50);
      }
   }
}

void Screen::clearCursor() {
   m_displayCursor = false;
   QWidget::update();
}

Screen::Result Screen::run() {
   lock_guard<mutex> lock(m_mutState);
   switch (m_state) {
   case State::Ready:
      m_status->setText(tr("Running"));
      m_state = State::Running;
      m_timerBlink->start(500);
      m_table->setEnabled(false);
      // notify before unlocking may block waiting thread immediately again ?
      m_cv.notify_one();
      return Result::Start;
   case State::Paused:
      m_status->setText(tr("Running"));
      m_state = State::Running;
      m_table->setEnabled(false);
      m_cv.notify_one();
      return Result::Resume;
   case State::Running:
      m_state = State::Paused;
      m_status->setText(tr("Paused"));
      m_table->setEnabled(true);
      clearCursor();
      return Result::Pause;
   default:
      assert(0);
   }
}

void Screen::stop() {
   {
      lock_guard<mutex> lock(m_mutState);
      m_state = State::Ready;
   }
   m_timerBlink->stop();
   m_table->setEnabled(true);
   clearCursor();
   m_status->setText(tr("Ready"));
}

void Screen::captureScreen() {
   static const uint8_t BMP_HEADER[62] = {
      0x42, 0x4D, 0x7E, 0x06, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x28, 0x00, 
      0x00, 0x00, 0xA0, 0x00, 0x00, 0x00, 0x50, 0x00,
      0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x40, 0x06, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
      0xFF, 0x00, 0x00, 0x00, 0x00, 0x00
   };
   
   auto datetime = QDateTime::currentDateTime().toLocalTime();
   auto date = datetime.date();
   auto time = datetime.time();
   
   char buf[1024];
   sprintf(buf, "%04d_%02d_%02d-%02d.%02d.%02d.%03d.bmp",
           date.year(), date.month(), date.day(),
           time.hour(), time.minute(), time.second(), time.msec());
   
   FILE *fp = fopen(buf, "wb");
   fwrite(BMP_HEADER, sizeof (BMP_HEADER), 1, fp);
   
   const uint8_t *p = m_gvb.device().getGraphBuffer();
   {
      lock_guard<mutex> lock(m_gvb.device().getGraphMutex());
      for (int i = 79; i >= 0; --i) {
         fwrite(p + i * 20, 20, 1, fp);
      }
   }
   fclose(fp);
   
   m_status->setText(tr("Saved screenshot as %1").arg(tr(buf)));
}

void Screen::blink() {
   if (m_gvb.device().cursorEnabled()) {
      lock_guard<mutex> lock(m_mutState);
      if (State::Running == m_state) {
         flipCursor();
         QWidget::update();
      }
   }
}

void Screen::keyDown(QKeyEvent *e) {
   auto it = s_wqxKeyMap.find(e->key());
   
   int key;
   if (it != s_wqxKeyMap.end()) {
      key = it->second;
   } else {
      key = 127;
   }
   
   char c;
   string s = e->text().toStdString();
   if (s.size()) {
      c = s[0];
   } else {
      c = 0;
   }
   
   if (key || c) {
      m_gvb.device().onKeyDown(key, c);
   }
}

void Screen::keyUp(QKeyEvent *e) {
   auto it = s_wqxKeyMap.find(e->key());
   if (it != s_wqxKeyMap.end()) {
      m_gvb.device().onKeyUp(it->second);
   }
}

Screen::~Screen() {
   {
      lock_guard<mutex> lock(m_mutState);
      m_state = State::Quit;
   }
   m_cv.notify_one();
   m_thread.join();
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
   // set minimum size
   window()->adjustSize();
   // disable resizing
   window()->setMaximumSize(window()->minimumSize());
}

void Screen::timerEvent(QTimerEvent *) {
   if (m_needUpdate.fetch_and(0)) {
      QWidget::update();
   }
}

void Screen::update() {
   // QWidget::update();
   m_needUpdate = 1;
}

void Screen::update(int x1, int y1, int x2, int y2) {
   // QWidget::update(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
   m_needUpdate = 1;
}

void Screen::sleep(int ticks) {
   if (ticks > 0) {
      QThread::usleep(ticks * m_sleepFactor);
   }
}

bool Screen::isStopped() {
   unique_lock<mutex> lock(m_mutState);
   
   if (State::Paused == m_state) {
      while (State::Paused == m_state)
         m_cv.wait(lock);
   }
   
   return State::Ready == m_state || State::Quit == m_state;
}

void Screen::beginInput() {
   emit showInputMethod();
}

void Screen::switchIM(InputMethod im) {
   switch (im) {
   case InputMethod::EN:
      m_im->setText(tr("[EN]"));
      break;
   case InputMethod::CH:
      m_im->setText(tr("[CH]"));
      break;
   }
}

void Screen::endInput() {
   emit hideInputMethod();
}