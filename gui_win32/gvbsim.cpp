#include <Windows.h>
#include <fstream>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <commctrl.h>
#include "resource.h"
#include "gvbsim.h"
#include "error.h"
#include "debug.h"
#include "ime.h"

#pragma comment (lib, "msimg32.lib")
#pragma comment (lib, "comctl32.lib")

using namespace std;

HINSTANCE hins;
const char *wcScrn = "sreen123", *wcKbd = "kbd123"; //窗口类
HWND hWnd, hscr, hkbd, hvar; //窗口，屏幕，键盘，变量listview
HMENU hmnu; //菜单
IME *ime;
POINT scrnpos; //屏幕相对于父窗口的坐标

byte image[16864], ascii[2560], gb[243648]; //字库
GVB g;
Device *device = 0;

 /*cs1:terminated用 cs2:同步mem(主要是key)用 cs3:input用*/
CRITICAL_SECTION cs1, cs2, cs3;
int state; //运行状态
const int S_NORM = 0, S_READY = 1, S_RUN = 2, S_PAUSE = 3;
const char *titles[] = {
    "gvbsim",
    "gvbsim [Ready]",
    "gvbsim [Running]",
    "gvbsim [Paused]"
};
const char *mnuPause = "暂停(&R)\t\tF5", *mnuRun = "运行(&R)\t\tF5";
HANDLE thread; //gvb线程
UINT timer;

OPENFILENAME ofn;
char fullfn[MAX_PATH + 1], initdir[MAX_PATH + 1];
string error; //错误信息

int x, y; //光标位置
bool cursor; //是否显示光标
int mode; //显示模式
byte mem[65536]; //memory
byte *tbuf = mem + 704; //文字缓冲
byte *gbuf = mem + 6592; //屏幕缓冲
bool enablecursor; //是否允许显示光标

bool inputing;
byte tmpchar[2]; //储存全角字符
string tmpstr;

PBITMAPINFO bmi;

/////////////////////////
struct {
    string s;
} modifyDlgVar; //两个对话框公用

struct {
    long index;
    Array *arry;
} modArryDlgVar;

struct {
    bool syntree;
    bool debuginfo;
    int delay;
} option;

int WINAPI WinMain(HINSTANCE hi, HINSTANCE, LPSTR, int nCmdShow) {
    hins = hi;
    //载入字库
    if (!loadFont()) {
        MessageBox(0, "字库文件 font.bin 打开失败！", "错误", MB_OK);
        return 0;
    }

    //窗口
    const char *wndcls1 = "lololo123";
    WNDCLASSEX wcx;

    wcx.cbSize = sizeof wcx;
    wcx.cbClsExtra = 0;
    wcx.cbWndExtra = 0;
    wcx.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
    wcx.hCursor = LoadCursor(0, IDC_ARROW);
    wcx.hIcon = LoadIcon(hins, MAKEINTRESOURCE(IDI_ICON1));
    wcx.hIconSm = 0;
    wcx.hInstance = hi;
    wcx.lpfnWndProc = wndProc;
    wcx.lpszClassName = wndcls1;
    wcx.lpszMenuName = 0;
    wcx.style = CS_DBLCLKS;
    if (!RegisterClassEx(&wcx)) {
        MessageBox(0, "窗口类1注册失败！", "错误", MB_OK);
        return 0;
    }

    //屏幕
    wcx.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wcx.hIcon = 0;
    wcx.lpfnWndProc = scrnProc;
    wcx.lpszClassName = wcScrn;
    RegisterClassEx(&wcx);
    //键盘
    wcx.lpfnWndProc = kbdProc;
    wcx.lpszClassName = wcKbd;
    RegisterClassEx(&wcx);

    //快捷键
    HACCEL hacl = LoadAccelerators(hi, MAKEINTRESOURCE(IDR_ACCELERATOR1));

    hWnd = CreateWindow(wndcls1, titles[S_NORM],
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        123, 123, 300, 300,
        0, hmnu = LoadMenu(hi, MAKEINTRESOURCE(IDR_MENU1)), hi, 0);
    //窗口居中
    RECT wnd, cln;

    GetClientRect(hWnd, &cln);
    GetWindowRect(hWnd, &wnd);

    int w = 300 + 330 + wnd.right - wnd.left - cln.right + cln.left,
        h = 324 + wnd.bottom - wnd.top - cln.bottom + cln.top,
        W = GetSystemMetrics(SM_CXSCREEN),
        H = GetSystemMetrics(SM_CYSCREEN);

    MoveWindow(hWnd, (W - w) / 2, (H - h) / 2, w, h, FALSE);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;

    while (GetMessage(&msg, 0, 0, 0)) {
        if (!TranslateAccelerator(hWnd, hacl, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return msg.wParam;
}

LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) { //窗口
    static int shotnum;

    switch (msg) {
    case WM_CREATE:
        CreateWindow("static", 0, WS_CHILD | WS_VISIBLE | SS_GRAYFRAME,
                4, 4, 322, 162, hwnd, (HMENU) 3, hins, 0);
        hscr = CreateWindow(wcScrn, 0, WS_CHILD | WS_VISIBLE,
                5, 5, 320, 160,
                hwnd, (HMENU) 1, hins, 0);
        hkbd = CreateWindow(wcKbd, 0, WS_CHILD | WS_VISIBLE,
                5, 184, 320, 132,
                hwnd, (HMENU) 2, hins, 0);

        hvar = CreateWindow(WC_LISTVIEW, 0, WS_CHILD | WS_VISIBLE
                | LVS_REPORT | LVS_SINGLESEL,
                332, 3, 293, 316,
                hwnd, (HMENU) 4, hins, 0);

        insertCols();

        //获取屏幕相对于父窗口的坐标
        RECT rc;
        GetWindowRect(hscr, &rc);
        scrnpos.x = rc.left;
        scrnpos.y = rc.top;
        ScreenToClient(hwnd, &scrnpos);
        //禁用菜单项
        EnableMenuItem(hmnu, IDM_RUN, MF_GRAYED);
        EnableMenuItem(hmnu, IDM_STOP, MF_GRAYED);
        //禁用输入法
        ime = new IME(hwnd);
        ime->disable();

        shotnum = 1;
        ZeroMemory(&option, sizeof option);

        cursor = enablecursor = false;
        device = new Dev;
        g.setDevice(device);
        state = S_NORM;
        thread = 0;
        timer = 0;
        InitializeCriticalSection(&cs1);
        InitializeCriticalSection(&cs3);
        GetCurrentDirectory(sizeof initdir, initdir);
        initOFN(hwnd);

        initDIB();

        return 0;
    case WM_CLOSE:
        if (thread) { //副线程中需要操作窗口，要在窗口销毁前结束线程
            EnterCriticalSection(&cs1);
            state = S_READY;
            LeaveCriticalSection(&cs1);
            ResumeThread(thread);
            waitThread();
            CloseHandle(thread);
        }
        delete ime;
        break;
    case WM_DESTROY:
        uninitDIB();

        delete device;
        DeleteCriticalSection(&cs1);
        DeleteCriticalSection(&cs3);
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN: case WM_KEYUP: case WM_CHAR: //键盘消息全都发送到虚拟键盘
        SendMessage(hkbd, msg, w, l);
        return 0;
    case WM_COMMAND:
        switch (LOWORD(w)) {
        case IDM_OPEN: //打开
            if (GetOpenFileName(&ofn)) {
                if (loadFile(fullfn)) { //ready
                    SetWindowText(hwnd, titles[state = S_READY]);
                    EnableMenuItem(hmnu, IDM_RUN, MF_ENABLED);
                    if (option.syntree) {
                        ofstream fout("debug.log");
                        dumpSyntaxTree(fout, g);
                        fout.close();
                    }
                }
                refresh();
            }
            break;
        case IDM_EXIT: //退出
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            break;
        case IDM_STOP: //停止
            EnterCriticalSection(&cs1);
            state = S_READY;
            LeaveCriticalSection(&cs1);
            ResumeThread(thread);
            waitThread();
            CloseHandle(thread);
            thread = 0;
            break;
        case IDM_RUN: // 运行/暂停
            EnterCriticalSection(&cs1);
            switch (state) {
            case S_READY: //开始
                SetWindowText(hwnd, titles[state = S_RUN]);
                SetFocus(hwnd);
                if (thread) {
                    CloseHandle(thread);
                }
                EnableMenuItem(hmnu, IDM_STOP, MF_ENABLED);
                EnableMenuItem(hmnu, IDM_OPEN, MF_GRAYED);
                ModifyMenu(hmnu, IDM_RUN, MF_STRING, IDM_RUN, mnuPause);
                error.clear();
                timer = SetTimer(0, 0, 500, blink);
                thread = CreateThread(0, 0, run, 0, 0, 0);
                EnableWindow(hvar, FALSE);
                break;
            case S_PAUSE: //继续
                SetWindowText(hwnd, titles[state = S_RUN]);
                ModifyMenu(hmnu, IDM_RUN, MF_STRING, IDM_RUN, mnuPause);
                ResumeThread(thread);
                timer = SetTimer(0, 0, 500, blink);
                EnableWindow(hvar, FALSE);
                SetFocus(hWnd);
                break;
            case S_RUN: //暂停
                SetWindowText(hwnd, titles[state = S_PAUSE]);
                ModifyMenu(hmnu, IDM_RUN, MF_STRING, IDM_RUN, mnuRun);
                SuspendThread(thread);
                KillTimer(0, timer);
                timer = 0;
                clearCursor();
                EnableWindow(hvar, TRUE);
                refreshItems();
            }
            LeaveCriticalSection(&cs1);
            break;
        case IDM_ABOUT: //关于
            {
                bool r = state == S_RUN;
                if (r) {
                    SendMessage(hwnd, WM_COMMAND, IDM_RUN, 0);
                }
                DialogBox(hins, MAKEINTRESOURCE(IDD_DIALOGABOUT), hwnd, aboutDlgProc);
                if (r) {
                    SendMessage(hwnd, WM_COMMAND, IDM_RUN, 0);
                }
            }
            break;
        case IDM_CAPTURE:
            {
                bool r = state == S_RUN;
                if (r) {
                    SendMessage(hwnd, WM_COMMAND, IDM_RUN, 0);
                }
                stringstream sout;
                sout << "screenshot_" << setw(3) << setfill('0') << shotnum++ << ".bmp";
                ofstream fout(sout.str().c_str(), ios_base::binary | ios_base::out);
                saveDIB(fout);
                fout.close();
                if (r) {
                    SendMessage(hwnd, WM_COMMAND, IDM_RUN, 0);
                }
            }
            break;
        case IDM_OPTION:
            {
                bool r = state == S_RUN;
                if (r) {
                    SendMessage(hwnd, WM_COMMAND, IDM_RUN, 0);
                }
                DialogBox(hins, MAKEINTRESOURCE(IDD_DIALOGOPT), hwnd, optionDlgProc);
                if (r) {
                    SendMessage(hwnd, WM_COMMAND, IDM_RUN, 0);
                }
            }
            break;
        }
        return 0;
    case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR) l;
            LPNMITEMACTIVATE ia;
            switch (hdr->code) {
            case NM_DBLCLK: //修改
                ia = (LPNMITEMACTIVATE) hdr;
                if (ia->iItem >= 0 && state != S_READY) {
                    char s[50], *_s;
                    ListView_GetItemText(hvar, ia->iItem, 2, s, 50);
                    modifyDlgVar.s = s;
                    GVB::variter it = g.getEnv().begin();
                    for (int i = ia->iItem; i > 0; i--) {
                        it++;
                    }
                    const IdKey ik = it->first;
                    V *val = it->second;
                    ListView_GetItemText(hvar, ia->iItem, 0, s, 50);
                    if (ik.type == V::ID) { //变量
                        if (DialogBoxParam(hins, MAKEINTRESOURCE(IDD_DIALOGINPUT), hWnd,
                                modifyDlgProc, (LPARAM) s)) {
                            ZeroMemory(s, 50);
                            modifyDlgVar.s.copy(s, 50);
                            //修改
                            double rt = 0;
                            long iit;
                            switch (val->vtype) {
                            case E::VREAL:
                                rt = strtod(s, &_s);
                                if (s != _s) {
                                    val->rval = rt;
                                    *_s = 0;
                                    ListView_SetItemText(hvar, ia->iItem, 2, s);
                                }
                                break;
                            case E::VINT:
                                iit = strtol(s, &_s, 10);
                                if (s != _s) {
                                    *_s = 0;
                                    val->ival = iit;
                                    ListView_SetItemText(hvar, ia->iItem, 2, s);
                                }
                                break;
                            case E::VSTRING:
                                val->sval = modifyDlgVar.s;
                                ListView_SetItemText(hvar, ia->iItem, 2, s);
                            }
                        }
                    } else { //修改数组
                        modArryDlgVar.arry = (Array *) val;
                        stringstream sout;
                        //标题
                        sout << s << '(';
                        for (int i = 0; i < modArryDlgVar.arry->bounds.size(); i++) {
                            sout << (modArryDlgVar.arry->bounds[i] - 1);
                            if (i < modArryDlgVar.arry->bounds.size() - 1)
                                sout << ", ";
                        }
                        sout << ')';
                        ZeroMemory(s, 50);
                        sout.str().copy(s, 50);
                        if (DialogBoxParam(hins, MAKEINTRESOURCE(IDD_DIALOGMODARRY), hWnd,
                                modArryDlgProc, (LPARAM) s)) {
                            ; //对话框中已修改
                        }
                    }
                }
                break;
            }
        }
        return 0;
    }
    return DefWindowProc(hwnd, msg, w, l);
}

LRESULT CALLBACK scrnProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) { //屏幕
    static HGDIOBJ tmpobj;
    static RECT tmprc;
    HDC hdc;
    PAINTSTRUCT ps;

    switch (msg) {
    case WM_CREATE:
        return 0;
    case WM_PAINT: //屏幕显示：图像，光标，错误信息
        hdc = BeginPaint(hwnd, &ps);
//{
//            char s[200];
//            wsprintf(s, "%d %d %d %d; %d %d %d %d\n",
//                ps.rcPaint.left, ps.rcPaint.top,
//                ps.rcPaint.right - ps.rcPaint.left,
//                ps.rcPaint.bottom - ps.rcPaint.top, 
//                ps.rcPaint.left >> 1, ps.rcPaint.top >> 1,
//                ((ps.rcPaint.right - ps.rcPaint.left) >> 1) ,
//                ((ps.rcPaint.bottom - ps.rcPaint.top) >> 1));
//            OutputDebugString(s);
//        }
        StretchDIBits(hdc, ps.rcPaint.left, ps.rcPaint.top,
                ps.rcPaint.right - ps.rcPaint.left,
                ps.rcPaint.bottom - ps.rcPaint.top,
                ps.rcPaint.left >> 1, 80 - (ps.rcPaint.bottom >> 1), //???
                (ps.rcPaint.right - ps.rcPaint.left) >> 1,
                (ps.rcPaint.bottom - ps.rcPaint.top) >> 1,
                gbuf, bmi, DIB_RGB_COLORS,
                SRCCOPY); //仅绘制无效区域
        
        if (cursor) { //显示光标
            int cx = x << 4, cy = y << 5, w = 16;
            RECT rect;

            switch (getPosInfo()) {
            case IS_GB_1:
                cx -= 16;
            case IS_GB_0:
                w <<= 1;
            }
            SetRect(&rect, cx, cy, cx + w, cy + 32);
            InvertRect(hdc, &rect);
        }
        if (error.length() > 0) { //显示错误信息
            tmpobj = SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));
            SetTextColor(hdc, RGB(255, 0, 0));
            GetClientRect(hwnd, &tmprc);
            DrawText(hdc, error.c_str(), -1, &tmprc, DT_WORDBREAK);
            SelectObject(hdc, tmpobj);
        }
        EndPaint(hwnd, &ps);
        return 0;
    case WM_KEYDOWN: case WM_KEYUP: case WM_CHAR:
        SendMessage(hkbd, msg, w, l);
        return 0;
    default:
        return DefWindowProc(hwnd, msg, w, l);
    }
}

LRESULT CALLBACK kbdProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) { //键盘
    static HBITMAP keyboard, key;
    static HDC hdckb, hdck;
    static HGDIOBJ objt1, objt2;
    static BLENDFUNCTION bf;
    static TRACKMOUSEEVENT tme;
    static bool tracked = false;

    static int mx = -1, my; //key highlight
    static bool pressed = false;
    static RECT keyrect;

    HDC hdc;
    PAINTSTRUCT ps;
    int _x, _y, tx, ty, t;
    bool r;

    switch (msg) {
    case WM_CREATE:
        mem[199] = 0;
        memset(mem + 191, 255, 8);

        keyboard = (HBITMAP) LoadImage(hins, MAKEINTRESOURCE(IDB_KEYBOARD),
                IMAGE_BITMAP, 320, 132, 0);
        key = (HBITMAP) LoadImage(hins, MAKEINTRESOURCE(IDB_KEY),
                IMAGE_BITMAP, 30, 24, 0);
        hdckb = CreateCompatibleDC(0);
        objt1 = SelectObject(hdckb, keyboard);
        hdck = CreateCompatibleDC(0);
        objt2 = SelectObject(hdck, key);
        ZeroMemory(&bf, sizeof (BLENDFUNCTION));
        bf.SourceConstantAlpha = 90;
        tme.cbSize = sizeof (TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE;
        tme.dwHoverTime = 0;
        tme.hwndTrack = hwnd;
        InitializeCriticalSection(&cs2);
        return 0;
    case WM_CLOSE:
        SelectObject(hdck, objt2);
        DeleteDC(hdck);
        SelectObject(hdckb, objt1);
        DeleteDC(hdckb);
        DeleteCriticalSection(&cs2);
        break;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        BitBlt(hdc, 0, 0, 320, 132, hdckb, 0, 0, SRCCOPY);
        EndPaint(hwnd, &ps);
        return 0;
    case WM_LBUTTONDOWN:
        if (mx >= 0) {
            hdc = GetDC(hwnd);
            keyrect.left = (mx << 5) + 1;
            keyrect.top = my * 26 + 2;
            keyrect.right = keyrect.left + 29;
            keyrect.bottom = keyrect.top + 23;
            InvertRect(hdc, &keyrect);
            pressed = true;
            ReleaseDC(hwnd, hdc);
            w = getKeyValue(mx, my);
            //PostMessage(hwnd, CM_KEYDOWN, w = getKeyValue(mx, my), 0);
            if (isprint(w)) {
                PostMessage(hwnd, WM_CHAR, w, 0);
            }
        }
        return 0;
    case WM_LBUTTONUP:
        if (pressed) {
            hdc = GetDC(hwnd);
            InvertRect(hdc, &keyrect);
            ReleaseDC(hwnd, hdc);
            pressed = false;
            PostMessage(hwnd, CM_KEYPRESS, w = getKeyValue(mx, my), 0);
            PostMessage(hwnd, CM_KEYUP, w, 0);
        }
        return 0;
    case WM_MOUSEMOVE:
        _x = (tx = LOWORD(l) - 1) >> 5;
        _y = (ty = HIWORD(l) - 2) / 26;
        if (!pressed) {
            if (tx >= 0 && (tx & 31) < 30 && ty >= 0 && ty % 26 < 24
                    && getKeyValue(_x, _y)) {
                if (_x != mx || _y != my) { //highlight
                    InvalidateRect(hwnd, 0, FALSE);
                    UpdateWindow(hwnd);
                    hdc = GetDC(hwnd);
                    mx = _x;
                    my = _y;
                    AlphaBlend(hdc, (_x << 5) + 1, _y * 26 + 2, 30, 24,
                            hdck, 0, 0, 30, 24,
                            bf);
                    ReleaseDC(hwnd, hdc);
                }
            } else if (mx >= 0) {
                InvalidateRect(hwnd, 0, FALSE);
                UpdateWindow(hwnd);
                mx = -1;
            }
        }
        if (!tracked) { //追踪mouseleave
            TrackMouseEvent(&tme);
            tracked = true;
        }
        return 0;
    case WM_MOUSELEAVE:
        mx = -1;
        InvalidateRect(hwnd, 0, FALSE);
        UpdateWindow(hwnd);
        tracked = false;
        pressed = false;
        return 0;
    case WM_KEYDOWN:
        //if (l & 0x40000000) { //禁止连续响应
        //    return 0;
        //}
        w = getKeyValue(w);
    case CM_KEYDOWN:
        t = w;
        if (t > 0) {
            PostMessage(hwnd, CM_KEYPRESS, t, 0); //按下就发送
            t = mappingKey(t);
            EnterCriticalSection(&cs2);
            mem[191 + (t >> 8)] &= ~(t & 0xff);
            LeaveCriticalSection(&cs2);
        }
        return 0;
    case WM_KEYUP:
        w = getKeyValue(w);
    case CM_KEYUP:
        t = w;
        if (t > 0) {
            t = mappingKey(t);
            EnterCriticalSection(&cs2);
            mem[191 + (t >> 8)] |= t & 0xff;
            LeaveCriticalSection(&cs2);
        }
        return 0;
    case CM_KEYPRESS: //自定义按键消息
        EnterCriticalSection(&cs2);
        mem[199] = 128 | w;
        LeaveCriticalSection(&cs2);
        if (w == 13) {
            EnterCriticalSection(&cs3);
            r = inputing;
            LeaveCriticalSection(&cs3);
            if (r) {
                device->nextLine();
                EnterCriticalSection(&cs3);
                inputing = false;
                LeaveCriticalSection(&cs3);
                mem[199] = 13;
            }
        }
        return 0;
    case WM_CHAR: //仅input用
        EnterCriticalSection(&cs3);
        r = inputing;
        LeaveCriticalSection(&cs3);
        if (r) {
            string ss;
            switch (w) {
            //case 13: //结束input
            //    device->nextLine();
            //    EnterCriticalSection(&cs3);
            //    inputing = false;
            //    LeaveCriticalSection(&cs3);
            //    break;
            case '\b':
                if (tmpstr.empty()) {
                    break;
                }
                if (tmpstr.back() & 0x80) {
                    tmpstr.pop_back();
                    tmpstr.pop_back();
                    x -= 2;
                    tbuf[y * 20 + x] = 32;
                    tbuf[y * 20 + x + 1] = 32;
                    if (x == 0 && tbuf[y * 20 - 1] == ' ' && tmpstr.back() != ' ') {
                        y--;
                        x = 19;
                    }
                } else {
                    if (--x < 0) {
                        y--;
                        x = 19;
                    }
                    tbuf[y * 20 + x] = 32;
                    tmpstr.pop_back();
                }
                device->updateLCD();
                ime->setCandidatePos((x << 4) + scrnpos.x, (y << 5) + scrnpos.y + 32);
                break;
            default:
                tmpchar[1] = w;
                if (w > 128) { //全角字符
                    if (*tmpchar) {
                        if (x >= 18 && y >= 4) {
                            *tmpchar = 0;
                            break;
                        }
                        ss = string(tmpchar, tmpchar + 2);
                        tmpstr += ss;
                        device->appendText(ss);
                        device->updateLCD();
                        ime->setCandidatePos((x << 4) + scrnpos.x,
                                (y << 5) + scrnpos.y + 32);
                        *tmpchar = 0;
                    } else {
                        *tmpchar = w;
                    }
                } else if (w > 31 && (y < 4 || x <= 18)) {
                    ss = string(tmpchar + 1, tmpchar + 2);
                    tmpstr += ss;
                    device->appendText(ss);
                    device->updateLCD();
                    ime->setCandidatePos((x << 4) + scrnpos.x, (y << 5) + scrnpos.y + 32);
                }
            }
        }
        return 0;
    }
    return DefWindowProc(hwnd, msg, w, l);
}

INT_PTR CALLBACK modifyDlgProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) { //修改变量对话框
    static HWND edit1;

    switch (msg) {
    case WM_INITDIALOG:
        SetWindowText(hwnd, (char *) l);
        SetFocus(edit1 = GetDlgItem(hwnd, IDC_EDIT1));
        SendDlgItemMessage(hwnd, IDC_EDIT1, EM_LIMITTEXT, 50, 0);
        SetWindowText(edit1, modifyDlgVar.s.c_str());
        return FALSE;
    case WM_COMMAND:
        switch (LOWORD(w)) {
        case IDOK:
            {
                char s[50];
                GetWindowText(edit1, s, 50);
                modifyDlgVar.s = s;
            }
            EndDialog(hwnd, TRUE);
            return TRUE;
        case IDCANCEL:
            EndDialog(hwnd, FALSE);
            return FALSE;
        }
        break;
    }
    return FALSE;
}

INT_PTR CALLBACK modArryDlgProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) { //修改数组对话框
    static HWND edit0;
    static int index[6], bound[6], size;

    switch (msg) {
    case WM_INITDIALOG:
        SetWindowText(hwnd, (char *) l);
        SendDlgItemMessage(hwnd, IDC_EDIT1, EM_LIMITTEXT, 50, 0);
        { //取数组第一个值
            stringstream sout;
            switch (modArryDlgVar.arry->vtype) {
            case E::VREAL:
                sout << modArryDlgVar.arry->rvals[0];
                break;
            case E::VINT:
                sout << modArryDlgVar.arry->ivals[0];
                break;
            case E::VSTRING:
                sout << modArryDlgVar.arry->svals[0];
            }
            modifyDlgVar.s = sout.str();
        }
        SetFocus(edit0 = GetDlgItem(hwnd, IDC_EDIT0));
        SetWindowText(edit0, modifyDlgVar.s.c_str());
        //设置updown控件
        size = modArryDlgVar.arry->bounds.size();
        for (int i = 0; i < 6; i++) {
            if (i < size) {
                SendDlgItemMessage(hwnd, i + IDC_SPIN1, UDM_SETBUDDY,
                        (LPARAM) GetDlgItem(hwnd, i + IDC_EDIT1), 0);
                SendDlgItemMessage(hwnd, i + IDC_SPIN1, UDM_SETRANGE, 0, (LPARAM)
                        MAKELONG((bound[i] = modArryDlgVar.arry->bounds[i]) - 1, 0));
                SendDlgItemMessage(hwnd, i + IDC_SPIN1, UDM_SETPOS, 0, index[i] = 0);
            } else {
                ShowWindow(GetDlgItem(hwnd, i + IDC_SPIN1), SW_HIDE);
                ShowWindow(GetDlgItem(hwnd, i + IDC_EDIT1), SW_HIDE);
            }
        }
        return FALSE;
    case WM_COMMAND:
        switch (LOWORD(w)) {
        case IDOK2:
            {
                char s[50], *_s;
                GetWindowText(edit0, s, 50);
                //取index
                int t = *index;
                for (int i = 1; i < size; i++) {
                    t = t * bound[i] + index[i];
                }
                modArryDlgVar.index = t;
                //修改
                double rt;
                long iit;
                switch (modArryDlgVar.arry->vtype) {
                case E::VREAL:
                    rt = strtod(s, &_s);
                    if (s != _s) {
                        *_s = 0;
                        modArryDlgVar.arry->rvals[t] = rt;
                        modifyDlgVar.s = s;
                    } else {
                        SetWindowText(edit0, modifyDlgVar.s.c_str());
                    }
                    break;
                case E::VINT:
                    iit = strtol(s, &_s, 10);
                    if (s != _s) {
                        *_s = 0;
                        modArryDlgVar.arry->ivals[t] = iit;
                        modifyDlgVar.s = s;
                    } else {
                        SetWindowText(edit0, modifyDlgVar.s.c_str());
                    }
                    break;
                case E::VSTRING:
                    modArryDlgVar.arry->svals[t] = modifyDlgVar.s = s;
                }
            }
            return TRUE;
        case IDOK0:
            EndDialog(hwnd, TRUE);
            return TRUE;
        case IDOK:
            {
                HWND tw = GetFocus();
                int t = GetDlgCtrlID(tw);
                if (t < IDC_EDIT1 || t > IDC_EDIT6) {
                    return TRUE;
                }
                int tt = GetDlgItemInt(hwnd, t, 0, FALSE);
                if (tt >= bound[t - IDC_EDIT1]) {
                    return TRUE;
                }
                index[t - IDC_EDIT1] = tt;
                t = *index;
                for (int i = 1; i < size; i++) {
                    t = t * bound[i] + index[i];
                }
                //取数组元素
                stringstream sout;
                switch (modArryDlgVar.arry->vtype) {
                case E::VREAL:
                    sout << modArryDlgVar.arry->rvals[t];
                    break;
                case E::VINT:
                    sout << modArryDlgVar.arry->ivals[t];
                    break;
                case E::VSTRING:
                    sout << modArryDlgVar.arry->svals[t];
                }
                modifyDlgVar.s = sout.str();
                SetWindowText(edit0, sout.str().c_str());
            }
            return TRUE;
        case IDCANCEL:
            EndDialog(hwnd, FALSE);
            return FALSE;
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR) l;
            switch (hdr->code) {
            case UDN_DELTAPOS:
                {
                    LPNMUPDOWN ud = (LPNMUPDOWN) l;
                    //取index
                    int i = hdr->idFrom - IDC_SPIN1;
                    index[i] = (ud->iPos + ud->iDelta + bound[i]) % bound[i];
                    int t = *index;
                    for (int i = 1; i < size; i++) {
                        t = t * bound[i] + index[i];
                    }
                    //取数组元素
                    stringstream sout;
                    switch (modArryDlgVar.arry->vtype) {
                    case E::VREAL:
                        sout << modArryDlgVar.arry->rvals[t];
                        break;
                    case E::VINT:
                        sout << modArryDlgVar.arry->ivals[t];
                        break;
                    case E::VSTRING:
                        sout << modArryDlgVar.arry->svals[t];
                    }
                    modifyDlgVar.s = sout.str();
                    SetWindowText(edit0, sout.str().c_str());
                }
                return 1;
            }
        }
        break;
    }
    return FALSE;
}

INT_PTR CALLBACK aboutDlgProc(HWND hwnd, UINT msg, WPARAM w, LPARAM) {
    switch (msg) {
    case WM_INITDIALOG:
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(w)) {
        case IDOK: case IDCANCEL:
            EndDialog(hwnd, 0);
            return TRUE;
        }
    }
    return FALSE;
}

INT_PTR CALLBACK optionDlgProc(HWND hwnd, UINT msg, WPARAM w, LPARAM) {
    switch (msg) {
    case WM_INITDIALOG:
        if (option.syntree) {
            SendDlgItemMessage(hwnd, IDC_CHECK1, BM_SETCHECK, BST_CHECKED, 0);
        }
        if (option.debuginfo) {
            SendDlgItemMessage(hwnd, IDC_CHECK2, BM_SETCHECK, BST_CHECKED, 0);
        }
        SendDlgItemMessage(hwnd, IDC_SPIN1, UDM_SETBUDDY,
                (LPARAM) GetDlgItem(hwnd, IDC_EDIT1), 0);
        SendDlgItemMessage(hwnd, IDC_SPIN1, UDM_SETRANGE, 0, 3000);
        SendDlgItemMessage(hwnd, IDC_SPIN1, UDM_SETPOS, 0, option.delay);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(w)) {
        case IDOK:
            option.syntree = SendDlgItemMessage(hwnd, IDC_CHECK1, BM_GETCHECK, 0, 0)
                    == BST_CHECKED;
            option.debuginfo = SendDlgItemMessage(hwnd, IDC_CHECK2, BM_GETCHECK, 0, 0)
                    == BST_CHECKED;
            option.delay = GetDlgItemInt(hwnd, IDC_EDIT1, 0, FALSE);
            EndDialog(hwnd, 0);
            return TRUE;
        case IDCANCEL:
            EndDialog(hwnd, 0);
            return TRUE;
        }
    }
    return FALSE;
}
/////////////////////////////////
bool terminated() { //是否强制结束
    EnterCriticalSection(&cs1);
    bool r = state == S_READY;
    LeaveCriticalSection(&cs1);
    return r;
}

void delay() { //延时
    if (option.delay > 0) {
        byte t = mem[65535];
        for (int i = 0; i < option.delay; i++) {
            byte a = 0;
            for (int j = 0; j < 200; j++) {
                a += mem[j];
            }
            mem[65535] ^= a;
        }
        mem[65535] = t;
    }
}
////////////////////////////////////获取光标位置信息
POS_INFO getPosInfo() {
    byte *j = tbuf + y * 20;
    POS_INFO r = IS_ASCII;

    for (int i = 0; i <= x; i++) {
        if (j[i] > 160) {
            if (r == IS_GB_0) {
                r = IS_GB_1;
            } else {
                r = IS_GB_0;
            }
        } else {
            r = IS_ASCII;
        }
    }
    return r;
}
/////////////////////////////////////
bool loadFont() { //载入字库
    /*ifstream fin("font.bin", ios_base::binary | ios_base::in);

    if (!fin.is_open()) {
        return false;
    }
    fin.read((char *) ascii, sizeof ascii);
    fin.read((char *) gb, sizeof gb);
    fin.read((char *) image, sizeof image);
    fin.close();*/
    HRSRC font = FindResource(0, MAKEINTRESOURCE(IDR_WQXFONT), "myfont");
    if (SizeofResource(0, font) == 0) {
        return false;
    }
    HGLOBAL hfnt = LoadResource(0, font);
    if (hfnt == 0) {
        return false;
    }
    char *pfnt = (char *) LockResource(hfnt);
    if (pfnt == 0) {
        FreeResource(hfnt);
        return false;
    }
    memcpy(ascii, pfnt, sizeof ascii);
    memcpy(gb, pfnt + sizeof ascii, sizeof gb);
    memcpy(image, pfnt + sizeof ascii + sizeof gb, sizeof image);
    FreeResource(hfnt);
    return true;
}

void initOFN(HWND hwnd) { //初始化OPENFILENAME结构
    memset(&ofn, 0, sizeof ofn);
    ofn.lStructSize = sizeof ofn;
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = "文本文件(*.txt)\0*.txt\0所有文件(*.*)\0*.*\0\0";
    ofn.lpstrFile = fullfn;
    ofn.nMaxFile = sizeof fullfn;
    ofn.lpstrInitialDir = initdir;
    ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
}

void initDIB() { //初始化dib
    bmi = (PBITMAPINFO) new char[sizeof (BITMAPINFOHEADER) + 2 * sizeof (RGBQUAD)];
    ZeroMemory(&bmi->bmiHeader, sizeof (BITMAPINFOHEADER));
    bmi->bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
    bmi->bmiHeader.biWidth = 160;
    bmi->bmiHeader.biHeight = -80;
    bmi->bmiHeader.biPlanes = 1;
    bmi->bmiHeader.biBitCount = 1;//颜色深度
    bmi->bmiHeader.biCompression = BI_RGB;
    bmi->bmiHeader.biSizeImage = (160 * 80) >> 3;
    *(int *) &bmi->bmiColors[0] = SCRNBGCOLOR;
    *(int *) &bmi->bmiColors[1] = SCRNFRCOLOR;

    //bmgbuf = CreateDIBSection(0, bmi, DIB_RGB_COLORS, (void **) &gbuf, 0, 0);
}

void uninitDIB() {
    //DeleteObject(bmgbuf);
    delete [] bmi;
}

bool loadFile(const char *fn) { //载入程序
    EnableMenuItem(hmnu, IDM_OPEN, MF_GRAYED); //open无效
    ifstream fin(fn);
    if (!fin.is_open()) {
        error = "文件打开失败";
        EnableMenuItem(hmnu, IDM_OPEN, MF_ENABLED);
        return false;
    }
    bool r = g.load(fin);
    if (!r) {
        error = "文件打开失败";
        fin.close();
        EnableMenuItem(hmnu, IDM_OPEN, MF_ENABLED);
        return false;
    }
    try {
        g.build();
    } catch (CE e) {
        error = e.description();
        EnableMenuItem(hmnu, IDM_OPEN, MF_ENABLED);
        return false;
    }
    fin.close();
    error.clear();
    EnableMenuItem(hmnu, IDM_OPEN, MF_ENABLED);
    return true;
}

void saveDIB(std::ostream &out) {
    BITMAPFILEHEADER bfh;

    bfh.bfSize = sizeof (BITMAPFILEHEADER) + sizeof (BITMAPINFOHEADER)
            + 2 * sizeof (RGBQUAD) + 1600;
    bfh.bfType = 0x4d42;
    bfh.bfReserved1 = bfh.bfReserved2 = 0;
    bfh.bfOffBits = bfh.bfSize - 1600;
    out.write((char *) &bfh, sizeof bfh);
    out.write((char *) bmi, sizeof (BITMAPINFOHEADER) + 2 * sizeof (RGBQUAD));
    out.write((char *) gbuf, 1600);
}

void dumpSyntaxTree(std::ostream &out, GVB &p) {
    show(p.getTree(), out);
}

void saveDebugInfo(std::ostream &out, GVB &p) {
    out << std::endl;
    show(p, out);
}

void insertCols() {
    static char *titles[] = {
        "变量名", "变量类型", "值"
    };
    static int widths[] = {
        80, 75, 120
    };

    LVCOLUMN lvc;
    for (int i = 0; i < 3; i++) {
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
        lvc.fmt = LVCFMT_LEFT;
        lvc.cx = widths[i];
        lvc.pszText = titles[i];
        ListView_InsertColumn(hvar, i, &lvc);
    }
}

void refreshItems() {
    SendMessage(hvar, WM_SETREDRAW, FALSE, 0);
    SendMessage(hvar, LVM_DELETEALLITEMS, 0, 0);

    LVITEM lvi;
    GVB::Env &vars = g.getEnv();
    int i = 0;
    stringstream s;
    ZeroMemory(&lvi, sizeof (LVITEM));
    for (GVB::variter vi = vars.begin(); vi != vars.end(); vi++) {
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;
        const IdKey &ik = vi->first;
        const V *val = vi->second;
        string ss = ik.id;
        for (string::iterator it = ss.begin(); it != ss.end(); it++) {
            *it = toupper(*it);
        }
        lvi.pszText = const_cast<char *>(ss.c_str());
        ListView_InsertItem(hvar, &lvi);
        ListView_SetItemText(hvar, i, 1,
                ik.type == V::ID && val->vtype == E::VREAL ? "实数" :
                ik.type == V::ID && val->vtype == E::VSTRING ? "字符串" :
                ik.type == V::ID && val->vtype == E::VINT ? "整数" :
                ik.type == V::ARRAY && val->vtype == E::VREAL ? "实数数组" :
                ik.type == V::ARRAY && val->vtype == E::VSTRING ? "字符串数组" :
                ik.type == V::ARRAY && val->vtype == E::VINT ? "整数数组" :
                "?");
        s.str("");
        if (ik.type == V::ID) {
            switch (val->vtype) {
            case E::VINT:
                s << val->ival;
                break;
            case E::VREAL:
                s << val->rval;
                break;
            case E::VSTRING:
                s << val->sval;
                break;
            default:
                s << '?';
            }
        } else {
            s << "...";
        }ss=s.str();
        ListView_SetItemText(hvar, i, 2, const_cast<char *>(ss.c_str()));
        i++;
    }
    SendMessage(hvar, WM_SETREDRAW, TRUE, 0);
}
/////////////////////////////////gvb线程
DWORD WINAPI run(LPVOID) { //不处理handle
    enablecursor = false;
    mem[199] = 0;
    memset(mem + 191, 255, 8);
    cursor = inputing = false;
    try {
        g.execute();
    } catch (RE r) {
        error = r.description();
    } catch (exception e) {
        error = e.what();
    }
    {
        char s[200];
        wsprintf(s, "line:%d label:%d\n", g.getCurrentLine(), g.getCurrentLabel());
        OutputDebugString(s);
    }

    state = S_READY;
    SetWindowText(hWnd, titles[state]);
    EnableMenuItem(hmnu, IDM_OPEN, MF_ENABLED);
    EnableMenuItem(hmnu, IDM_STOP, MF_GRAYED);
    ModifyMenu(hmnu, IDM_RUN, MF_STRING, IDM_RUN, mnuRun);
    if (timer) {
        KillTimer(0, timer);
        timer = 0;
    }
    clearCursor();

    //输出debug info
    if (option.debuginfo) {
        ofstream fout("debug.log", ios_base::app);
        saveDebugInfo(fout, g);
        fout.close();
    }

    EnableWindow(hvar, TRUE);

    return 0;
}

void waitThread() {
    MSG msg;
    while (true) {
        if (MsgWaitForMultipleObjects(1, &thread, FALSE, INFINITE, QS_ALLINPUT)
                == WAIT_OBJECT_0) {
            break;
        } else {
            /* 副线程run(LPVOID)需要借助消息循环完成任务，WaitForSingleObject时主线程无法处理
            消息，导致副线程无法结束，主线程阻塞。因此主线程需要边等待边处理消息
            */
            PeekMessage(&msg, 0, 0, 0, PM_REMOVE);
            DispatchMessage(&msg); 
        }
    }
}

void CALLBACK blink(HWND, UINT, UINT_PTR, DWORD) {
    if (mode == Device::TEXT && enablecursor) {
        cursor = !cursor;
        refresh();
    }
}

void refresh() {
    InvalidateRect(hscr, 0, FALSE);
}

void refresh(int left, int top, int right, int bottom) {
    RECT r;
    //{
    //    char s[100];
    //    wsprintf(s,"r:%d %d %d %d\n", left,top,right,bottom);
    //    OutputDebugString(s);
    //}
    r.left = left << 1;
    r.top = top << 1;
    r.right = (right << 1) + 2;
    r.bottom = (bottom << 1) + 2;
    InvalidateRect(hscr, &r, FALSE);
}

void clearCursor() {
    cursor = false;
    refresh();
}

int keyval[] = {
    28, 29, 30, 31, 0, 0, 0, 0, 0, 0,
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 13,
    'z', 'x', 'c', 'v', 'b', 'n', 'm', 19, 20, 14,
    25, 26, 18, 27, '0', '.', ' ', 23, 21, 22
};
int getKeyValue(int x, int y) {
    return keyval[y * 10 + x];
}

/* [`]=求助 [Shift]=Shift [Ctrl]=CAPS [Esc]=Esc
*/
int pckeyval[] = {
    112, 113, 114, 115, -1, -1, -1, -1, -1, -1,
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 13,
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', 33, 38, 34,
    192, 16, 17, 27, 48, 190, 32, 37, 40, 39
};
int getKeyValue(int pckey) {
    for (int i = 0; i < sizeof pckeyval; i++) {
        if (pckey == pckeyval[i]) {
            return keyval[i];
        }
    }
    switch (pckey) {
    case 96: return '0';
    case 97: case '1': return 'b';
    case 98: case '2': return 'n';
    case 99: case '3': return 'm';
    case 100: case '4': return 'g';
    case 101: case '5': return 'h';
    case 102: case '6': return 'j';
    case 103: case '7': return 't';
    case 104: case '8': return 'y';
    case 105: case '9': return 'u';
    case 110: return '.';
    default: return 0;
    }
}

int keyMap[] = { //tc808
    0x702, -1, 0x602, -1, 0x502, -1, 0x402, -1, //0
    0x302, -1, 0x202, -1, 0x102, 0x208, 0x108, 0x304, //8
    0x204, -1, 0x580, 0x040, 0x508, 0x408, 0x008, 0x080, //16
    0x701, 0x780, 0x680, 0x480, 0x704, 0x604, 0x504, 0x404, //24
    0x180, -1, -1, -1, -1, -1, -1, -1, //32
    -1, -1, -1, -1, -1, -1, 0x280, -1, //40
    0x380, -1, -1, -1, -1, -1, -1, -1, //48
    -1, -1, -1, -1, -1, -1, -1, -1, //56
    -1, -1, -1, -1, -1, -1, -1, -1, //64
    -1, -1, -1, -1, -1, -1, -1, -1, //72
    -1, -1, -1, -1, -1, -1, -1, -1, //80
    -1, -1, -1, -1, -1, -1, -1, -1, //88
    -1, 0x720, 0x340, 0x540, 0x520, 0x510, 0x420, 0x320, //96
    0x220, 0x010, 0x120, 0x020, 0x608, 0x140, 0x240, 0x708, //104
    0x308, 0x710, 0x410, 0x620, 0x310, 0x110, 0x440, 0x610, //112
    0x640, 0x210, 0x740 //120
};
int mappingKey(int wqxkey) {
    return keyMap[wqxkey];
}
//////////////////////////////////Dev类
const int Dev::bmask[] = {0x80, 0x40, 0x20, 0x10, 8, 4, 2, 1};

Dev::Dev() {
    mode = TEXT;
    x = y = 0;
}

void Dev::moveLine() {
    memmove(tbuf, tbuf + 20, 80);
    memset(tbuf + 80, 0, 20);
    y--;
    memmove(gbuf, gbuf + 320, 1600 - 320);
    memset(gbuf + 1600 - 320, 0, 320);
}

void Dev::appendText(const string &s) {
    for (int i = 0; i < s.length(); i++) {
        if ((byte) s[i] > 160 && x == 19) {
            tbuf[y * 20 + 19] = 32;
            x = 0;
            y++;
        }
        if (y >= 5) {
            moveLine();
        }
        tbuf[y * 20 + x++] = (byte) s[i];
        if ((byte) s[i] > 160 && i < s.length() - 1) {
            tbuf[y * 20 + x++] = (byte) s[++i];
        }
        if (x >= 20) {
            y++;
            x = 0;
        }
    }

    int i;
    for (i = x; i < 20; i++) {
        if (tbuf[y * 20 + i]) {
            break;
        }
    }
    byte *t = tbuf + y * 20 + i;
    for (; i < 20; i++) {
        if (!*t) {
            break;
        }
        *t++ = 0;
    }
    if (y >= 5) {
        moveLine();
    }
}

void Dev::nextLine() {
    x = 0;
    y++;
    if (y >= 5) {
        moveLine();
    }
}

void Dev::updateLCD() {
    if (mode == TEXT) {
        memset(gbuf, 0, 1600);
    }
    byte *c = tbuf, *f, *g, t; //f字库 g屏幕缓冲
    long ff;

    for (int j = 0; j < 5; j++) {
        for (int i = 0; i < 20; i++) {
            g = j * 320 + i + gbuf;
            if (*c > 160) {
                ff = (*c << 8) | (t = c[1]);
                if (ff >= 0xf8a1 && ff <= 0xfdd9 && t > 160) { //wqx图形
                    f = image + (((*c - 248) * 94 + t - 161) << 5);
                } else {
                    ff = *c - 161;
                    if (ff > 8)
                        ff -= 6;
                    ff = (ff * 94 + t - 161) << 5;
                    if (ff < 0 || ff + 32 > sizeof gb) {
                        ff = 85 << 5; //黑方块
                    }
                    f = gb + ff;
                }
                if (i++ != 19) {
                    for (int k = 0; k < 16; k++) {
                        *g = *f++;
                        g[1] = *f++;
                        g += 20;
                    }
                } else { //汉字位于行尾时分成两半显示...
                    for (int k = 0; k < 16; k++) {
                        *g = *f++;
                        g[301] = *f++;
                        g += 20;
                    }
                    j++;
                    i = 0;
                }
                c++;
            } else if (*c) {
                f = *c * 16 + ascii;
                for (int k = 0; k < 16; k++) {
                    *g = *f++;
                    g += 20;
                }
            }
            c++;
        }
    }
    refresh();
}

void Dev::locate(int row, int col) {
    x = col;
    y = row;
    if (mode == TEXT) {
        refresh();
    }
}

int Dev::getX() {
    return x;
}

int Dev::getY() {
    return y;
}

void Dev::setMode(int mode) {
    if ((::mode = mode) == TEXT) {
        cursor = false;
    }
    cls();
}

void Dev::cls() {
    memset(tbuf, 0, 100);
    memset(gbuf, 0, 1600);
    x = y = 0;
    refresh();
}

string Dev::input(const string &prompt, int type) {
    enablecursor = true;
    ime->enable();
    int tx = (x << 4) + scrnpos.x, ty = (y << 5) + scrnpos.y;
    ime->setCandidatePos(tx, ty + 32);
    *tmpchar = 0;
    tmpstr.clear();
    EnterCriticalSection(&cs3);
    inputing = true;
    LeaveCriticalSection(&cs3);

    while (true) {
        EnterCriticalSection(&cs3);
        bool r = inputing;
        LeaveCriticalSection(&cs3);
        if (terminated()) {
            ime->disable();
            throw 1;
        }
        if (!r) {
            break;
        }
    }
    ime->disable();
    enablecursor = cursor = false;
    return tmpstr;
}

int Dev::getkey() {
    enablecursor = true;
    int a = -1;
    while (a < 0) {
        EnterCriticalSection(&cs2);
        a = mem[199] - 128;
        LeaveCriticalSection(&cs2);
        if (terminated()) {
            throw 1;
        }
        Sleep(20);
    }
    enablecursor = cursor = false;
    EnterCriticalSection(&cs2);
    mem[199] &= 127;
    LeaveCriticalSection(&cs2);
    return a;
}

void Dev::setPoint(Dev::coord x, Dev::coord y) {
    if (x > 159 || y > 79) {
        return;
    }
    byte *o = gbuf + y * 20 + (x >> 3);
    switch (tmode) {
    case PAINT:
        *o |= bmask[x & 7];
        break;
    case CLEAR:
        *o &= ~bmask[x & 7];
        break;
    case INVERSE:
        *o ^= bmask[x & 7];
    }
}

void Dev::point(Dev::coord x, Dev::coord y, int mode) {
    tmode = mode;
    setPoint(x, y);
    refresh(x, y, x, y);
}

void Dev::hLine(Dev::coord x1, Dev::coord x2, Dev::coord y) {
    for (; x1 <= x2; x1++) {
        setPoint(x1, y);
    }
}

void Dev::rectangle(coord x1, coord y1, coord x2, coord y2, bool fill, int mode) {
    tmode = mode;
    if (x1 > x2) {
        int t = x1;
        x1 = x2;
        x2 = t;
    }
    if (y1 > y2) {
        int t = y1;
        y1 = y2;
        y2 = t;
    }
    if (x2 > 159)
        x2 = 159;
    if (y2 > 79)
        y2 = 79;

    int y1_ = y1;
    if (fill) {
        for (; y1 <= y2; y1++)
            hLine(x1, x2, y1);
    } else {
        hLine(x1, x2, y1);
        hLine(x1, x2, y2);
        for (; y1 <= y2; y1++) {
            setPoint(x1, y1);
            setPoint(x2, y1);
        }
    }
    refresh(x1, y1_, x2, y2);
}

void Dev::line(Dev::coord x1, Dev::coord y1, Dev::coord x2, Dev::coord y2, int mode) {
    tmode = mode;
    if (x1 > x2) {
        int t = x1;
        x1 = x2;
        x2 = t;
        t = y1;
        y1 = y2;
        y2 = t;
    }
        
    int dx = x2 - x1, dy = y2 - y1, sgn = 1, tx = 0, ty = 0, x_ = x1, y_ = y1;
    if (dy < 0) {
        dy = -dy;
        sgn = -1;
    }
    int m = dx > dy ? dx : dy, i = m;
    while (i-- >= 0) {
        setPoint(x1, y1);
        tx += dx;
        ty += dy;
        if (tx >= m) {
            x1++;
            tx -= m;
        }
        if (ty >= m) {
            y1 += sgn;
            ty -= m;
        }
    }
    refresh(x_, min(y_, y2), x2, max(y_, y2));
}

void Dev::ovalPoint(Dev::coord ox, Dev::coord oy, Dev::coord x, Dev::coord y) {
    setPoint(ox - x, oy - y);
    setPoint(ox - x, oy + y);
    setPoint(ox + x, oy - y);
    setPoint(ox + x, oy + y);
}

void Dev::ellipse(Dev::coord x, Dev::coord y, Dev::coord rx, Dev::coord ry,
        bool fill, int mode) {
    tmode = mode;
    int asq = rx * rx, bsq = ry * ry;
    int asq2 = asq * 2, bsq2 = bsq * 2;
    int p;
    int x1 = 0, y1 = ry;
    int px = 0, py = asq2 * y1;
    p = bsq - asq * ry + ((asq + 2) >> 2);
    while (px < py) {
        x1++;
        px += bsq2;
        if (p < 0) {
            p += bsq + px;
        } else {
            if (fill) {
                hLine(x - x1 + 1, x + x1 - 1, y + y1);
                hLine(x - x1 + 1, x + x1 - 1, y - y1);
            }
            y1--;
            py -= asq2;
            p += bsq + px - py;
        }
        if (!fill) {
            ovalPoint(x, y, x1, y1);
        }

    }
    if (fill) {
        hLine(x - x1, x + x1, y + y1);
        hLine(x - x1, x + x1, y - y1);
    }
    p = bsq * x1 * x1 + bsq * x1 + asq * (y1 - 1) * (y1 - 1) - asq * bsq + ((bsq + 2) >> 2);
    while (--y1 > 0) {
        py -= asq2;
        if (p > 0) {
            p += asq - py;
        } else {
            x1++;
            px += bsq2;
            p += asq - py + px;
        }
        if (fill) {
            hLine(x - x1, x + x1, y + y1);
            hLine(x - x1, x + x1, y - y1);
        } else {
            ovalPoint(x, y, x1, y1);
        }
    }
    if (fill) {
        hLine(x - rx, x + rx, y);
    } else {
        setPoint(x, y + ry);
        setPoint(x, y - ry);
        setPoint(x + rx, y);
        setPoint(x - rx, y);
    }
    refresh(x - rx, y - ry, x + rx, y + ry);
}

int val__, x__, y__;

int Dev::peek(int addr) {
    EnterCriticalSection(&cs2);
    val__ = mem[addr & 0xffff];
    LeaveCriticalSection(&cs2);
    return val__;
}

string Dev::peek(int addr, int size) {
    string s;
    EnterCriticalSection(&cs2);
    for (int i = 0; i < size; i++) {
        s.push_back(mem[addr++ & 0xffff]);
    }
    LeaveCriticalSection(&cs2);
    return s;
}

void Dev::poke(int addr, byte value) {
    EnterCriticalSection(&cs2);
    mem[addr & 0xffff] = value;
    LeaveCriticalSection(&cs2);
    if (addr >= 6592 && addr <= 8191) {
        addr -= 6592;
        x__ = addr % 20;
        y__ = addr / 20;
        refresh(x__ << 3, y__, (x__ + 1) << 3, y__);
    } else if (addr == 199 && value == 155) {
        throw 1;
    }
}

void Dev::poke(int addr, const char *s, int size) {
    EnterCriticalSection(&cs2);
    for (int i = 0; i < size; i++) {
        mem[addr++ & 0xffff] = s[i];
    }
    LeaveCriticalSection(&cs2);
    refresh();
}