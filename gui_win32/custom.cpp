#include <Windows.h>
#include "gvbsim.h"

void Dev::sleep(int ms) { //延时
    if (ms > 0) {
        DWORD ms0 = GetTickCount();
        while (GetTickCount() - ms0 < ms) {
            if (terminated()) {
                throw 1;
            }
        }
    }
}

static byte maskl[] = {0, 0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f},
    maskr[] = {0, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe};

//内存块移位，bit>0右移，<0左移
static void rollData(void *data, int size, int bit) { //bit绝对值必须小于8
    if (!bit)
        return;
    byte *d = (byte *) data, c, t = 0;
    if (bit > 0) { //右移
        int rb = 8 - bit;
        for (int i = 0; i < size; i++) {
            c = *d;
            *d++ = (c >> bit) | t;
            t = (c << rb) & maskr[bit];
        }
    } else { //左移
        bit = -bit;
        int rb = 8 - bit;
        for (int i = 1; i < size; i++) {
            d++;
        }
        for (int i = 0; i < size; i++) {
            c = *d;
            *d-- = (c << bit) | t;
            t = (c >> rb) & maskl[bit];
        }
    }
}

void Dev::paint(int addr, int x, int y, byte w, byte h, int mode) {
    if (x > 159 || y > 79 || x + w < 0 || y + h < 0) {
        return;
    }
    int bw = (w + 7) >> 3;
    //每行开始第一个数据前无用的bit数
    int unuseDataBits = 0;
    if (x < 0) {
        addr += (-x) / 8;
        unuseDataBits = (-x) % 8;
        w += x;
        x = 0;
    }
    if (y < 0) {
        addr += -bw * y;
        h += y;
        y = 0;
    }
    if (x + w > 160) {
        w -= x + w - 160;
    }
    if (y + h > 80) {
        h -= y + h - 80;
    }

    extern byte *gbuf;
    int h_ = h;
    //绘制处前无用的bit数
    int unuseScreenBits = x % 8;
    //绘制开始地址
    int offset = 20 * y + x / 8;
    //实际每行用到数据的byte数
    int count = (unuseDataBits + w + 7) / 8;
    //实际绘制影响到的byte数
    int size = (unuseScreenBits + w + 7) / 8;
    //绘制结尾剩下的bit数
    int remain = size * 8 - unuseScreenBits - w;
    //用于存储图像数据
    char mapData[22];
    while (h-- > 0) {
        for (int i = 0; i < count; i++) {
            mapData[i] = peek(addr + i);
        }
        addr += bw;
        rollData(mapData, count + 1, unuseScreenBits - unuseDataBits);
        for (int i = 0; i < size; i++) {
            int s = mapData[i], d = gbuf[offset + i];
            int mask = 0;
            if (i == 0) {
                mask |= maskr[unuseScreenBits];
            }
            if (i == size - 1) {
                mask |= maskl[remain];
            }
            s &= ~mask;
            d &= ~mask;
            gbuf[offset + i] &= mask;
            switch (mode) {
            case _NOT:
                s ^= ~mask;
                break;
            case _OR:
                s |= d;
                break;
            case _AND:
                s &= d;
                break;
            case _XOR:
                s ^= d;
            }
            gbuf[offset + i] |= s;
        }
        offset += 20;
    }
    refresh(x, y, x + w, y + h_);
}

bool Dev::getPoint(int x, int y) {
    extern byte *gbuf;
    if (x < 0 || y < 0 || x > 159 || y > 79) {
        return true;
    }
    return gbuf[y * 20 + (x >> 3)] & bmask[x & 7];
}

bool Dev::checkKey(int keycode) {
    extern byte mem[];
    extern CRITICAL_SECTION cs2;
    int t = mappingKey(keycode);

    EnterCriticalSection(&cs2);
    bool r = mem[191 + (t >> 8)] & (t & 0xff);
    LeaveCriticalSection(&cs2);
    return !r;
}