#pragma once

#include <string>
#include <vector>

using std::string;
using std::vector;

struct O {
    O *pre, *nxt;

    static long n;

    O() { n++; }
    virtual ~O() = 0 { n--; }
};

struct Node : O {
    int line, label, type;

    static long n; //检查mem leak用

    Node(int lin, int lbl, int t) : line(lin), label(lbl), type(t) {}
    virtual ~Node() = 0 {}
};

//空结点，用于gc
struct Null : O {
    Null() {}
};

struct S : Node {
    enum {
        NONE, CLS, DIM, ASSIGN, SWAP, IF, ON, FOR, NEXT, WHILE, WEND, DEF,
        GOSUB, RETURN, READ, RESTORE, INPUT, FINPUT, PRINT, LOCATE, INKEY, GRAPH, TEXT,
        DRAW, LINE, BOX, CIRCLE, ELLIPSE, OPEN, CLOSE, PUT, GET, LSET, RSET, POP, CLEAR,
        WRITE, POKE, CALL, END, FIELD, GOTO, INVERSE, DEFFN,
        SLEEP, PAINT, LOAD, FREAD, FWRITE, FSEEK, FPUTC
    };
    S *next;
    S(int line, int label, int type) : Node(line, label, type), next(0) {}
    virtual ~S() = 0 {};
};

struct E : Node {
    int vtype; //表达式值类型
    enum {
        BINARY = 1, FUNC, CLFN, ID, ACCESS, INKEY,
        CON_MASK = 0x20, //常量mask,用于常量折叠
        STRING, REAL,
    };
    enum { //表达式值类型
        VREAL = 1, VSTRING,
        RVAL_MASK = 0x1f,
        VINT = 0x21
    };

    static string typeDesc(int);
    E(int line, int label, int type, int vt)
        : Node(line, label, type), vtype(vt) {}
    virtual ~E() = 0 {};
};

struct Binary : E { //二元式
    E *l, *r;
    int op;
    Binary(int line, int label, int op, E *l, E *r, int vt)
        : E(line, label, BINARY, vt), op(op), l(l), r(r) {}
};

struct Str : E { //字符串常量
    string sval;
    Str(int line, int label, const string &s)
        : E(line, label, STRING, VSTRING), sval(s) {}
};

struct Real : E { //实数常量
    double rval;
    Real(int line, int label, double d) : E(line, label, REAL, VREAL), rval(d) {}
};

struct Func : E { //函数
    E *x, *x2, *x3; //mid的x3可能为0
    int f;
    enum {
        NONE, ABS, ASC, ATN, CHR, COS, CVI, MKI, CVS, MKS, EXP, INT, LEFT, LEN, LOG, MID,
        POS, RIGHT, RND, SGN, SIN, SQR, STR, TAN, VAL, PEEK, FEOF, LOF, TAB, SPC,
        NEG, NOT,
        POINT, CHECKKEY, FOPEN, FGETC, FTELL
    };

    static int getFuncType(int);
    static int getFuncParamType(int, int);
    Func(int line, int label, E *x, int f, int vt)
        : E(line, label, FUNC, vt), x(x), x2(0), x3(0), f(f) {}
    Func(int line, int label, E *x, E *x2, int f, int vt)
        : E(line, label, FUNC, vt), x(x), x2(x2), x3(0), f(f) {}
    Func(int line, int label, E *x, E *x2, E *x3, int f, int vt)
        : E(line, label, FUNC, vt), x(x), x2(x2), x3(x3), f(f) {}
};

struct CallFn : E { //自定义函数
    E *x;
    string fn;
    CallFn(int line, int label, E *x, const string &f, int vt)
        : E(line, label, CLFN, vt), x(x), fn(f) {}
};

struct Id : E {
    string id;
    Id(int line, int label, const string &i, int vtype)
        : E(line, label, ID, vtype), id(i) {}
    Id(int line, int label, int t, const string &i, int vtype)
        : E(line, label, t, vtype), id(i) {}
};

struct Access : Id {
    vector<E *> index;
    Access(int line, int label, const string &id, const vector<E *> v, int vt)
        : Id(line, label, ACCESS, id, vt), index(v) {}
};

struct Inkey : E {
    Inkey(int l, int lb) : E(l, lb, INKEY, VSTRING) {}
};

struct None : S {
    None(int line, int label) : S(line, label, NONE) {}
    None(int l, int lb, int t) : S(l, lb, t) {}
};

struct If : S {
    E *con;
    S *stm, *elsestm;

    If(int line, int label, E *c, S *s, S *els)
        : S(line, label, IF), con(c), stm(s), elsestm(els) {}
};

struct Call : S {
    E *addr;

    Call(int l, int lb, E *addr) : S(l, lb, CALL), addr(addr) {}
};

struct Assign : S {
    Id *id;
    E *val;

    Assign(int l, int lb, Id *i, E *e) : S(l, lb, ASSIGN), id(i), val(e) {}
};

struct Goto : S {
    int gotol;
    S *gotos;

    Goto(int l, int lb, S *lbl, int ll) : S(l, lb, GOTO), gotos(lbl), gotol(ll) {}
};

struct Gosub : S {
    int gotol;
    S *gotos;

    Gosub(int l, int lb, S *lbl, int ll) : S(l, lb, GOSUB), gotos(lbl), gotol(ll) {}
};

struct DefFn : S {
    string x, f;
    int xvtype, vtype;
    E *fn;

    DefFn(int l, int lb, const string &f, int vtype, const string &x, int xvtype, E *fn)
        : S(l, lb, DEFFN), x(x), fn(fn), f(f), xvtype(xvtype), vtype(vtype) {}
};

struct Print : S {
    vector<E *> ps; //可能有0
    vector<int> del;
    enum { //分隔符
        NO, CR
    };

    Print(int l, int lb) : S(l, lb, PRINT) {}
};

struct For : S {
    string v;
    E *dest, *step; //step可能为0

    For(int l, int lb, const string &v, E *d, E *s)
        : S(l, lb, FOR), v(v), dest(d), step(s) {}
};

struct Next : S {
    string v;

    Next(int l, int lb, const string &id) : S(l, lb, NEXT), v(id) {}
    Next(int l, int lb) : S(l, lb, NEXT) {}
};

struct Dim : S {
    Id *id;

    Dim(int l, int lb, Id *i) : S(l, lb, DIM), id(i) {}
};

struct While : S {
    E *con;

    While(int l, int lb, E *c) : S(l, lb, WHILE), con(c) {}
};

struct Input : S {
    string prm;
    vector<Id *> ids;

    Input(int l, int lb, const string &s) : S(l, lb, INPUT), prm(s) {}
    Input(int l, int lb) : S(l, lb, INPUT) {}
};

struct FInput : S {
    int fnum; //012
    vector<Id *> ids;

    FInput(int l, int lb, int filenum) : S(l, lb, FINPUT), fnum(filenum) {}
};

struct Write : S {
    int fnum; //012
    vector<E *> es;

    Write(int l, int lb, int filenum) : S(l, lb, WRITE), fnum(filenum) {}
};

struct Close : S {
    int fnum;

    Close(int l, int lb, int filenum) : S(l, lb, CLOSE), fnum(filenum) {}
};

struct Open : S {
    int fnum;
    E *fname;
    int mode, len; //len random模式用
    enum {
        MINPUT = 1, MOUTPUT, MRANDOM, MAPPEND, MBINARY
    };
    enum {
        NOLEN = -1
    };

    Open(int l, int lb, int filenum, E *filename, int m, int ln)
        : S(l, lb, OPEN), fnum(filenum), fname(filename), mode(m), len(ln) {}
};

struct Locate : S {
    E *row, *col; //row可能为0

    Locate(int l, int lb, E *r, E *c) : S(l, lb, LOCATE), col(c), row(r) {}
};

struct Swap : S {
    Id *id1, *id2;

    Swap(int l, int lb, Id *i1, Id *i2) : S(l, lb, SWAP), id1(i1), id2(i2) {}
};

struct Read : S {
    vector<Id *> ids;

    Read(int l, int lb) : S(l, lb, READ) {}
};

struct Poke : S {
    E *addr, *val;

    Poke(int l, int lb, E *ad, E *v) : S(l, lb, POKE), addr(ad), val(v) {}
};

struct Restore : S {
    int rlabel;
    enum {
        NOLABEL = -1
    };

    Restore(int l, int lb, int rl) : S(l, lb, RESTORE), rlabel(rl) {}
};

struct On : S {
    E *con;
    vector<int> labels;
    vector<S *> stms;
    bool isSub;

    On(int l, int lb, E *c, bool sub) : S(l, lb, ON), con(c), isSub(sub) {}
};

struct Draw : S {
    E *x, *y, *t; //t可能为0

    Draw(int l, int lb, E *x, E *y, E *t) : S(l, lb, DRAW), x(x), y(y), t(t) {}
};

struct Line : S {
    E *x1, *y1, *x2, *y2, *t;

    Line(int l, int lb, E *x1, E *y1, E *x2, E *y2, E *t)
        : S(l, lb, LINE), x1(x1), y1(y1), x2(x2), y2(y2), t(t) {}
};

struct Box : S {
    E *x1, *y1, *x2, *y2, *f, *t; //f,t可能为0

    Box(int l, int lb, E *x1, E *y1, E *x2, E *y2, E *f, E *t)
        : S(l, lb, BOX), x1(x1), y1(y1), x2(x2), y2(y2), t(t), f(f) {}
};

struct Circle : S {
    E *x, *y, *r, *f, *t;

    Circle(int l, int lb, E *x, E *y, E *r, E *f, E *t)
        : S(l, lb, CIRCLE), x(x), y(y), r(r), f(f), t(t) {}
};

struct Ellipse : S {
    E *x, *y, *xr, *yr, *f, *t;

    Ellipse(int l, int lb, E *x, E *y, E *xr, E *yr, E *f, E *t)
        : S(l, lb, ELLIPSE), x(x), y(y), xr(xr), yr(yr), t(t), f(f) {}
};

struct Put : S {
    int fnum;
    E *rec;

    Put(int l, int lb, int filenum, E *r) : S(l, lb, PUT), fnum(filenum), rec(r) {}
};

struct Get : S {
    int fnum;
    E *rec;

    Get(int l, int lb, int filenum, E *r) : S(l, lb, GET), fnum(filenum), rec(r) {}
};

struct Field : S {
    int fnum, total;
    vector<int> bys;
    vector<string> ids; //id$

    Field(int l, int lb, int fnum) : S(l, lb, FIELD), fnum(fnum) {}
};

struct Lset : S {
    Id *id;
    E *s;

    Lset(int l, int lb, Id *id, E *s) : S(l, lb, LSET), id(id), s(s) {}
};

struct Rset : S {
    Id *id;
    E *s;

    Rset(int l, int lb, Id *id, E *s) : S(l, lb, RSET), id(id), s(s) {}
};

//////////////////////////
struct SSleep : S {
    E *ms;

    SSleep(int l, int lb, E *ms) : S(l, lb, SLEEP), ms(ms) {}
};

struct SPaint : S {
    E *addr, *x, *y, *w, *h, *mode;

    SPaint(int l, int lb, E *addr, E *x, E *y, E *w, E *h, E *mode)
        : S(l, lb, PAINT), addr(addr), x(x), y(y), w(w), h(h), mode(mode) {}
};

struct SLoad : S {
    E *addr;
    string values;

    SLoad(int l, int lb, E *addr, const string &values)
        : S(l, lb, LOAD), addr(addr), values(values) {}
};

struct SFputc : S {
    int fnum;
    E *ch;

    SFputc(int l, int lb, int fnum, E *ch) : S(l, lb, FPUTC), fnum(fnum), ch(ch) {}
};

struct SFread : S {
    int fnum;
    E *addr, *size;

    SFread(int l, int lb, int fnum, E *addr, E *size)
        : S(l, lb, FREAD), fnum(fnum), addr(addr), size(size) {}
};

struct SFwrite : S {
    int fnum;
    E *addr, *size;

    SFwrite(int l, int lb, int fnum, E *addr, E *size)
        : S(l, lb, FWRITE), fnum(fnum), addr(addr), size(size) {}
};

struct SFseek : S {
    int fnum;
    E *pt;

    SFseek(int l, int lb, int fnum, E *pt)
        : S(l, lb, FSEEK), fnum(fnum), pt(pt) {}
};