#include "lex.h"
#include <cstdlib>
#include <cctype>

void Lexer::loadKeywords() {
    kw["dim"] = DIM; kw["let"] = LET; kw["swap"] = SWAP; kw["goto"] = GOTO;
    kw["if"] = IF; kw["then"] = THEN; kw["else"] = ELSE; kw["on"] = ON;
    kw["for"] = FOR; kw["to"] = TO; kw["next"] = NEXT; kw["step"] = STEP;
    kw["while"] = WHILE; kw["wend"] = WEND; kw["def"] = DEF; kw["fn"] = FN;
    kw["gosub"] = GOSUB; kw["return"] = RETURN; kw["not"] = NOT; kw["and"] = AND;
    kw["or"] = OR; kw["read"] = READ; kw["data"] = DATA; kw["restore"] = RESTORE;
    kw["input"] = INPUT; kw["print"] = PRINT; kw["locate"] = LOCATE; kw["inverse"] = INVERSE;
    kw["inkey$"] = INKEY; kw["play"] = PLAY; kw["beep"] = BEEP; kw["graph"] = GRAPH;
    kw["text"] = TEXT; kw["draw"] = DRAW; kw["line"] = LINE; kw["box"] = BOX;
    kw["circle"] = CIRCLE; kw["ellipse"] = ELLIPSE; kw["open"] = OPEN; kw["close"] = CLOSE;
    kw["put"] = PUT; kw["get"] = GET; kw["lset"] = LSET; kw["rset"] = RSET;
    kw["cont"] = CONT; kw["pop"] = POP; kw["rem"] = REM; kw["clear"] = CLEAR;
    kw["write"] = WRITE; kw["as"] = AS; kw["poke"] = POKE; kw["call"] = CALL;
    kw["cls"] = CLS; kw["field"] = FIELD; kw["end"] = END; kw["tab"] = TAB;
    kw["spc"] = SPC;
    kw["sleep"] = SLEEP;kw["paint"] = PAINT;kw["load"] = LOAD;
    kw["fputc"] = FPUTC;kw["fseek"] = FSEEK;kw["fwrite"] = FWRITE;kw["fread"] = FREAD;
}

int Lexer::peek() {
    return c = (in.eof() ? -1 : in.get());
}

bool Lexer::peek(int ch) {
    if (peek() != ch)
        return false;
    peek();
    return true;
}

int Lexer::getc() {
    int d = c;
    peek();
    return d;
}

void Lexer::skipTo(char ch) {
    sval.clear();
    while (c != ch && c != -1) {
        sval += c;
        peek();
    }
}

void Lexer::skipSpace() {
    while (c == ' ') {
        peek();
    }
}

int Lexer::getToken() {
    while (c == ' ' || c == 0xd) {
        peek();
    }
    switch (c) {
    case 0xa:
        peek();
        return tok = 0xa;
    case '<':
        if (peek('>')) {
            return tok = NEQ;
        } else if (c == '=') {
            peek();
            return tok = LE;
        }
        return tok = '<';
    case '>':
        if (peek('='))
            return tok = GE;
        return tok = '>';
    case '"':
        sval.clear();
        while (peek() != '"' && c != 0xd && c != -1 && c != 0xa) {
            sval += c;
        }
        if (c == '"')
            peek();
        return tok = STRING;
    }
    if (c >= '0' && c <= '9' || c == '.') {
        sval.clear();
        while (c >= '0' && c <= '9') {
            sval += c;
            peek();
        }
        if (c != '.' && c != 'E' && c != 'e' && sval.length() < 10) { //超过9位当作浮点数
            ival = std::atol(sval.c_str());
            return tok = INT;
        }
        if (c == '.') {
            do {
                sval += c;
            } while (peek() >= '0' && c <= '9');
            if (sval.length() == 1) {
                return tok = '.';
            }
        }
        if (c == 'E' || c == 'e') {
            sval += 'E';
            peek();
            if (c == '+' || c == '-') {
                sval += c;
                peek();
            }
            while (c >= '0' && c <= '9') {
                sval += c;
                peek();
            }
        }
        const char *pp = sval.c_str();
        char *ppp;
        rval = std::strtod(pp, &ppp);
        if (ppp - pp != sval.length()) {
            throw 1; ///////////////////////
        }
        return tok = REAL;
    }
    if ((c | 0x20) >= 'a' && (c | 0x20) <= 'z') {
        sval.clear();
        do {
            sval += tolower(c);
        } while ((peek() | 0x20) >= 'a' && (c | 0x20) <= 'z' || c >= '0' && c <= '9');
        if (c == '%' || c == '$') {
            sval += c;
            peek();
        }
        kwiter i = kw.find(sval);
        if (i == kw.end()) {
            return tok = ID;
        }
        return tok = i->second;
    }
    tok = c;
    peek();
    return tok;
}