#pragma once

#include <string>
#include <hash_map>
#include <istream>
#include "util.h"

using std::istream;
using std::hash_map;

class Lexer {
private:
    hash_map<string, int, StringHash> kw;
    int c;
    istream &in;

    int peek();
    bool peek(int);

protected:
    void loadKeywords();
    typedef hash_map<string, int, StringHash>::iterator kwiter;

public:
    enum {
        ID = 256, INT, REAL, STRING, DIM, LET, SWAP, GOTO, IF, THEN, ELSE, ON, FOR, TO,
        NEXT, STEP, WHILE, WEND, DEF, FN, GOSUB, RETURN, AND, OR, NOT, READ, DATA,
        RESTORE, INPUT, PRINT, LOCATE, INVERSE, INKEY, PLAY, BEEP, GRAPH, TEXT, DRAW,
        LINE, BOX, CIRCLE, ELLIPSE, OPEN, CLOSE, PUT, GET, LSET, RSET, CONT, POP, REM,
        CLEAR, WRITE, AS, POKE, CALL, CLS, FIELD, END, GE, LE, NEQ, TAB, SPC,
        SLEEP, PAINT, LOAD, FPUTC, FREAD, FWRITE, FSEEK
    };

    int tok;
    long ival;
    double rval;
    string sval;

    int getc();
    void skipSpace();
    void skipTo(char); //保留char,跳过的字符串放在sval
    int getToken(); //throws Exception

    Lexer(istream &i) : in(i), c(' '), tok(0) { loadKeywords(); }
    //Lexer(const char *) : in(){}
    ~Lexer() {}
};

typedef Lexer L;