#include "gvbsim.h"

typedef unsigned short word;

extern byte mem[];

static byte n, v, b, d, i, z, c;
static byte a, x, y, s;
static word pc, ut;
static byte bt;
//static word ut;

//更新屏幕
static bool needUpdate;
static int rclx, rcly, rcrx, rcry, it, itx, ity;

/////////////////////////设置P
static void sn(byte b) { //bit7=1则n=1
    n = b >> 7;
}

static void sv(word w) { //（有符号）bit8 ^ bit7则v=1
    v = (((w >> 1) ^ w) & 0x80) != 0;
}

static void sz(byte b) {
    z = b == 0;
}

static void sc(word w) { //（无符号）bit8=1则c=1
    c = (w >> 8) & 1;
}
//////////////////////////////////////
static byte getb() {//获取一个字节
    return mem[pc++];
}

static word getw() {//获取一个字
    return pc+=2,*(word*)(mem+pc-2);
}

static byte &getbz() { //零页寻址，数据
    return mem[getb()];
}

static byte &getbd() { //直接寻址，数据
    return mem[getw()];
}

static byte &getbxi() { //先变址X间址，数据
    return mem[*(word *) (mem + x + getb())];
}

static byte &getbyi() { //后变址Y间址，数据
    return mem[*(word*)(mem+getb())+y];
}

static byte &getbxd() { //直接x变址，数据
    return mem[getw()+x];
}

static byte &getbyd() { //直接y变址，数据
    return mem[getw()+y];
}

static byte &getbxz() { //零页x变址，数据
    return mem[getb()+x];
}

static byte &getbyz() { //零页y变址，数据
    return mem[getb()+y];
}

static word &getwi() { //间接寻址，地址
    return *(word*)(mem+getw());
}

static void branch(byte b) {
    if (b) {
        bt = mem[pc];
        if (bt & 0x80) { //neg
            pc += bt - 255;
        } else {
            pc += 1 + bt;
        }
    } else
        pc++;
}

static byte popb() { //弹出一个字节
    return mem[++s|0x100];
}

static void pushb(byte b) { //压入一个字节
    mem[s--|0x100] = b;
}

static word popw() { //弹出一个字
    s += 2;
    return *(word *) (mem + s + 0xff);
}

static void pushw(word w) { //压入一个字
    *(word *) (mem + s + 0xff) = w;
    s -= 2;
}

static word sgn(byte b) { //无符号变为有符号，双符号位
    return (signed char) b;
}

static byte &recordRegion(byte &b) {
    it = &b - mem;
    if (it >= 6592 && it <= 8191) {
        needUpdate = true;
        it -= 6592;
        itx = it % 20;
        ity = it / 20;
        if (itx < rclx) {
            rclx = itx;
        }
        if (itx > rcrx) {
            rcrx = itx;
        }
        if (ity < rcly) {
            rcly = ity;
        }
        if (ity > rcry) {
            rcry = ity;
        }
    }
    return b;
}

/////////////////////////指令
static void adc(byte b) {
    if (!d) {
        ut = sgn(a) + c + sgn(b);
        sv(ut);
        a = ut = a + c + b;
        sc(ut);sn(a);sz(a);
    } else { //不支持bcd模式
        //bcd模式下不影响nz
    }
}

static void sbc(byte b) {
    if (!d) {
        bt = a;
        c = !c;
        a = ut = sgn(a) - sgn(b) - c;
        sn(a);sz(a);sv(ut);c=bt>=b+c;
    } else {
    }
}

static void inc(byte &b) {
    b++;
    sn(b);sz(b);
}

static void dec(byte &b) {
    b--;
    sn(b);sz(b);
}

static void and(byte b) {
     a &= b;
     sn(a);sz(a);
}

static void ora(byte b) {
     a |= b;
     sn(a);sz(a);
}

static void eor(byte b) {
    a ^= b;
    sn(a);sz(a);
}

static void cmp(byte b) {
    sn(sgn(a) - sgn(b));
    z = a == b;
    c = a >= b;
}

static void cpx(byte b) {
    sn(sgn(x) - sgn(b));
    z = x == b;
    c = x >= b;
}

static void cpy(byte b) {
    sn(sgn(y) - sgn(b));
    z = y == b;
    c = y >= b;
}

static void bit(byte b) {
    z = (a ^ b) == 0;
    n = b >> 7;
    v = (b >> 6) & 1;
}

static void asl(byte &b) {
    c = b >> 7;
    b <<= 1;
    sn(b);sz(b);
}

static void lsr(byte &b) {
    c = b & 1;
    n = 0;
    b >>= 1;
    sz(b);
}

static void rol(byte &b) {
    bt = c;
    c = b >> 7;
    b = (b << 1) | bt;
    sn(b);sz(b);
}

static void ror(byte &b) {
    bt = c;
    c = b & 1;
    b >>= 1;
    if (bt) {
        b |= 0x80;
    }
    sn(b);sz(b);
}

static byte compressP() {
    return (n << 7) | (v << 6) | (b << 4) | (d << 3) | (i << 2) | (z << 1) | c;
}

static void expandP(byte b) {
    n = b >> 7;
    v = (b >> 6) & 1;
    ::b = (b >> 4) & 1;
    d = (b >> 3) & 1;
    i = (b >> 2) & 1;
    z = (b >> 1) & 1;
    c = b & 1;
}

/*处理中断
inum    中断号
*/
static void handleInt(word inum) {
}

/////////////////////////////////////
void Dev::call(int addr) {
    bool flag = true;

    a = 0, x = 0, y = 0, s = 0xff;
    n = 0, v = 0, b = 0, d = 0, i = 0, z = 0, c = 0;
    pushw(0xffff);
    pc = addr;

    needUpdate = false;
    rclx = 20;
    rcly = 80;
    rcrx = rcry = 0;

    while (flag) {
        if (terminated()) {
            throw 1;
        }
        switch (mem[pc++]) {
        case 96:
            pc = popw();
            if (pc == 0xffff) {
                flag = false;
            }
            break;
        case 0:
            flag = false;//若处理中断，要把这句去掉
            ut = getw(); ///////中断号
            handleInt(ut);
            break;
        case 0xa1: //lda (xx,x)
            a = getbxi();
            sn(a);sz(a);
            break;
        case 0xa5: //lda xx
            a = getbz();
            sn(a);sz(a);
            break;
        case 0xa9: //lda #xx
            a = getb();
            sn(a);sz(a);
            break;
        case 0xad: //lda xxxx
            a = getbd();
            sn(a);sz(a);
            break;
        case 0xb1: //lda (xx),y
            a=getbyi();
            sn(a);sz(a);
            break;
        case 0xb5: //lda xx,x
            a=getbxz();
            sn(a);sz(a);
            break;
        case 0xb9: //lda xxxx,y
            a=getbyd();
            sn(a);sz(a);
            break;
        case 0xbd: //lda xxxx,x
            a=getbxd();
            sn(a);sz(a);
            break;
        case 0xa2: //ldx #xx
            x=getb();
            sn(x);sz(x);
            break;
        case 0xa6: //ldx xx
            x=getbz();
            sn(x);sz(x);
            break;
        case 0xae: //ldx xxxx
            x=getbd();
            sn(x);sz(x);
            break;
        case 0xb6: //ldx xx,y
            x=getbyz();
            sn(x);sz(x);
            break;
        case 0xbe: //ldx xxxx,y
            x=getbyd();
            sn(x);sz(x);
            break;
        case 0xa0: //ldy #xx
            y=getb();
            sn(y);sz(y);
            break;
        case 0xa4: //ldy xx
            y=getbz();
            sn(y);sz(y);
            break;
        case 0xac: //ldy xxxx
            y=getbd();
            sn(y);sz(y);
            break;
        case 0xb4: //ldy xx,x
            y=getbxz();
            sn(y);sz(y);
            break;
        case 0xbc: //ldy xxxx,x
            y=getbxd();
            sn(y);sz(y);
            break;
        case 0x81: //sta (xx,x)
            recordRegion(getbxi()) = a;
            break;
        case 0x85: //sta xx
            recordRegion(getbz()) = a;
            break;
        case 0x8d: //sta xxxx
            recordRegion(getbd()) = a;
            break;
        case 0x91: //sta (xx),y
            recordRegion(getbyi()) = a;
            break;
        case 0x95: //sta xx,x
            recordRegion(getbxz()) = a;
            break;
        case 0x99: //sta xxxx,y
            recordRegion(getbyd()) = a;
            break;
        case 0x9d: //sta xxxx,x
            recordRegion(getbxd()) = a;
            break;
        case 0x86: //stx xx
            recordRegion(getbz()) = x;
            break;
        case 0x8e: //stx xxxx
            recordRegion(getbd()) = x;
            break;
        case 0x96: //stx xx,y
            recordRegion(getbyz()) = x;
            break;
        case 0x84: //sty xx
            recordRegion(getbz()) = y;
            break;
        case 0x8c: //sty xxxx
            recordRegion(getbd()) = y;
            break;
        case 0x94: //sty xx,x
            recordRegion(getbxz()) = y;
            break;
        case 0xaa: //tax
            x = a;
            sn(x);sz(x);
            break;
        case 0x8a: //txa
            a=x;sn(a);sz(a);
            break;
        case 0xa8: //tay
            y=a;sn(y);sz(y);
            break;
        case 0x98: //tya
            a=y;sn(a);sz(a);
            break;
        case 0xba: //tsx
            x=s;sn(x);sz(x);
            break;
        case 0x9a: //txs
            s=x;
            break;
        case 0x61: //adc (xx,x)
            adc(getbxi());
            break;
        case 0x65: //adc xx
            adc(getbz());
            break;
        case 0x69: //adc #xx
            adc(getb());
            break;
        case 0x6d: //adc xxxx
            adc(getbd());
            break;
        case 0x71: //adc (xx), y
            adc(getbyi());
            break;
        case 0x75: //adc xx,x
            adc(getbxz());
            break;
        case 0x79: //adc xxxx,y
            adc(getbyd());
            break;
        case 0x7d: //adc xxxx,x
            adc(getbxd());
            break;
        case 0xe6: //inc xx
            inc(recordRegion(getbz()));
            break;
        case 0xee: //inc xxxx
            inc(recordRegion(getbd()));
            break;
        case 0xf6: //inc xx,x
            inc(recordRegion(getbxz()));
            break;
        case 0xfe: //inc xxxx,x
            inc(recordRegion(getbxd()));
            break;
        case 0xc6: //dec xx
            dec(recordRegion(getbz()));
            break;
        case 0xce: //dec xxxx
            dec(recordRegion(getbd()));
            break;
        case 0xd6: //dec xx,x
            dec(recordRegion(getbxz()));
            break;
        case 0xde: //dec xxxx,x
            dec(recordRegion(getbxd()));
            break;
        case 0xe8: //inx
            inc(x);
            break;
        case 0xca: //dex
            dec(x);
            break;
        case 0xc8: //iny
            inc(y);
            break;
        case 0x88: //dey
            dec(y);
            break;
        case 0x21: //and (xx, x)
            and(getbxi());
            break;
        case 0x25: //and xx
            and(getbz());
            break;
        case 0x29: //and #xx
            and(getb());
            break;
        case 0x2d: //and xxxx
            and(getbd());
            break;
        case 0x31: //and (xx),y
            and(getbyi());
            break;
        case 0x35: //and xx,x
            and(getbxz());
            break;
        case 0x39: //and xxxx,y
            and(getbyd());
            break;
        case 0x3d: //and xxxx,x
            and(getbxd());
            break;
        case 0x1: //ora (xx,x)
            ora(getbxi());
            break;
        case 0x5: //ora xx
            ora(getbz());
            break;
        case 0x9: //ora #xx
            ora(getb());
            break;
        case 0xd: //ora xxxx
            ora(getbd());
            break;
        case 0x11: //ora (xx),y
            ora(getbyi());
            break;
        case 0x15: //ora xx,x
            ora(getbxz());
            break;
        case 0x19: //ora xxxx,y
            ora(getbyd());
            break;
        case 0x1d: //ora xxxx,x
            ora(getbxd());
            break;
        case 0x41: //eor (xx,x)
            eor(getbxi());
            break;
        case 0x45: //eor xx
            eor(getbz());
            break;
        case 0x49: //eor #xx
            eor(getb());
            break;
        case 0x4d: //eor xxxx
            eor(getbd());
            break;
        case 0x51: //eor (xx),y
            eor(getbyi());
            break;
        case 0x55: //eor xx,x
            eor(getbxz());
            break;
        case 0x59: //eor xxxx,y
            eor(getbyd());
            break;
        case 0x5d: //eor xxxx,x
            eor(getbxd());
            break;
        case 0x18: //clc
            c = 0;
            break;
        case 0x38: //sec
            c = 1;
            break;
        case 0xd8: //cld
            d = 0;
            break;
        case 0xf8: //sed
            d = 1;
            break;
        case 0xb8: //clv
            v = 0;
            break;
        case 0x58: //cli
            i = 0;
            break;
        case 0x78: //sei
            i = 1;
            break;
        case 0xc1: //cmp (xx,x)
            cmp(getbxi());
            break;
        case 0xc5: //cmp xx
            cmp(getbz());
            break;
        case 0xc9: //cmp #xx
            cmp(getb());
            break;
        case 0xcd: //cmp xxxx
            cmp(getbd());
            break;
        case 0xd1: //cmp (xx),y
            cmp(getbyi());
            break;
        case 0xd5: //cmp xx,x
            cmp(getbxz());
            break;
        case 0xd9: //cmp xxxx,y
            cmp(getbyd());
            break;
        case 0xdd: //cmp xxxx,x
            cmp(getbxd());
            break;
        case 0xe0: //cpx #xx
            cpx(getb());
            break;
        case 0xe4: //cpx xx
            cpx(getbz());
            break;
        case 0xec: //cpx xxxx
            cpx(getbd());
            break;
        case 0xc0: //cpy #xx
            cpy(getb());
            break;
        case 0xc4: //cpy xx
            cpy(getbz());
            break;
        case 0xcc: //cpy xxxx
            cpy(getbd());
            break;
        case 0x24: //bit xx
            bit(getbz());
            break;
        case 0x2c: //bit xxxx
            bit(getbd());
            break;
        case 0xa: //asl
            asl(a);
            break;
        case 0x6: //asl xx
            asl(recordRegion(getbz()));
            break;
        case 0xe: //asl xxxx
            asl(recordRegion(getbd()));
            break;
        case 0x16: //asl xx,x
            asl(recordRegion(getbxz()));
            break;
        case 0x1e: //asl xxxx,x
            asl(recordRegion(getbxd()));
            break;
        case 0x4a: //lsr
            lsr(a);
            break;
        case 0x46: //lsr xx
            lsr(recordRegion(getbz()));
            break;
        case 0x4e: //lsr xxxx
            lsr(recordRegion(getbd()));
            break;
        case 0x56: //lsr xx,x
            lsr(recordRegion(getbxz()));
            break;
        case 0x5e: //lsr xxxx,x
            lsr(recordRegion(getbxd()));
            break;
        case 0x2a: //rol
            rol(a);
            break;
        case 0x26: //rol xx
            rol(recordRegion(getbz()));
            break;
        case 0x2e: //rol xxxx
            rol(recordRegion(getbd()));
            break;
        case 0x36: //rol xx,x
            rol(recordRegion(getbxz()));
            break;
        case 0x3e: //rol xxxx,x
            rol(recordRegion(getbxd()));
            break;
        case 0x6a: //ror
            ror(a);
            break;
        case 0x66: //ror xx
            ror(recordRegion(getbz()));
            break;
        case 0x6e: //ror xxxx
            ror(recordRegion(getbd()));
            break;
        case 0x76: //ror xx,x
            ror(recordRegion(getbxz()));
            break;
        case 0x7e: //ror xxxx,x
            ror(recordRegion(getbxd()));
            break;
        case 0x48: //pha
            mem[s-- | 0x100] = a;
            break;
        case 0x68: //pla
            a = mem[++s | 0x100];
            break;
        case 0x8: //php
            pushb(compressP());
            break;
        case 0x28: //plp
            expandP(popb());
            break;
        case 0x4c: //jmp xxxx
            pc = getw();
            break;
        case 0x5c: //jmp (xxxx)
            pc = getwi();
            break;
        case 0xf0: //beq
            branch(z);
            break;
        case 0xd0: //bne
            branch(!z);
            break;
        case 0xb0: //bcs
            branch(c);
            break;
        case 0x90: //bcc
            branch(!c);
            break;
        case 0x30: //bmi
            branch(n);
            break;
        case 0x10: //bpl
            branch(!n);
            break;
        case 0x70: //bvs
            branch(v);
            break;
        case 0x50: //bvc
            branch(!v);
            break;
        case 0x40: //rti
            expandP(popb());
            b = 0;
            pc = popw();
            break;
        case 0xea: //nop
            break;
        case 0x20: //jsr
            ut = getw();
            pushw(pc);
            pc = ut;
            break;
        }
    }
    if (needUpdate) {
        refresh(rclx << 3, rcly, (rcrx + 1) << 3, rcry);
    }
}