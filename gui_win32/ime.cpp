#include "ime.h"

IME::IME(HWND h) : himc(0) {
    setHandle(h);
}

IME::~IME() {
    if (hwnd) {
        ImmAssociateContext(hwnd, 0);
        ImmReleaseContext(hwnd, himc);
    }
}

void IME::disable() { //disable后就无法enable???
    if (hwnd) {
        //himc = ImmAssociateContext(hwnd, 0);
    }
}

void IME::enable() {
    if (hwnd) {
        //ImmAssociateContext(hwnd, himc);
    }
}

void IME::setHandle(HWND h) {
    if (hwnd = h) {
        /*ImmGetContext无法获取其他进程的input context
        */
        himc = ImmGetContext(hwnd);
    }
}

void IME::setCandidatePos(int x, int y) {
    CANDIDATEFORM cf;

    ZeroMemory(&cf, sizeof (CANDIDATEFORM));
    cf.dwStyle = CFS_EXCLUDE; //??
    cf.ptCurrentPos.x = x;
    cf.ptCurrentPos.y = y;
    ImmSetCandidateWindow(himc, &cf);
}

void IME::setCandidatePos(int x, int y, int rl, int rt, int rr, int rb) {
    CANDIDATEFORM cf;

    cf.dwIndex = 0;
    cf.dwStyle = CFS_EXCLUDE;
    cf.ptCurrentPos.x = x;
    cf.ptCurrentPos.y = y;
    cf.rcArea.left = rl;
    cf.rcArea.top = rt;
    cf.rcArea.right = rr;
    cf.rcArea.bottom = rb;
    ImmSetCandidateWindow(himc, &cf);
}