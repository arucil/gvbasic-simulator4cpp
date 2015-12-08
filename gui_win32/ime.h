#pragma once

#include <Windows.h>
#include <imm.h>

#pragma comment (lib, "imm32.lib")

class IME {
    HWND hwnd;
    HIMC himc;
public:
    IME(HWND);
    IME() : hwnd(0), himc(0) {}
    ~IME();
    void enable(); //启用输入法
    void disable(); //禁用输入法
    void setHandle(HWND); //设置窗口句柄
    void setCandidatePos(int x, int y); //设置候选字窗口位置(client)
    void setCandidatePos(int x, int y, int rl, int rt, int rr, int rb);
};