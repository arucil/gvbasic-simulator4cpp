#include <fstream>
#include <cstring>
#include <sstream>
#include "file.h"

using std::ofstream;
using std::ifstream;
using std::fstream;
using std::ios_base;
using std::stringstream;

File::~File() {
    close();
}

void File::open(const string &fn, int fm) {
    if (f)
        return;
    fname = fn;
    mode = fm;
    p = 0;
    buf = new char[capacity = DEFAULT_BUFFER_SIZE];

    switch (mode) {
    case FIN:
        if (!readFile()) {
            delete [] buf;
            buf = 0;
            return;
        }
        break;
    case FOUT:
        {
            ofstream o(fname);
            if (!o.is_open()) {
                delete [] buf;
                buf = 0;
                return;
            }
            o.close();
        }
        break;
    case FAPP:
        if (!readFile()) {
            fstream o(fname.c_str(), ios_base::app);
            if (!o.is_open()) {
                delete [] buf;
                buf = 0;
                return;
            }
            o.close();
        }
        p = length;
        break;
    case FRAN:
        if (!readFile()) {
            fstream o(fname.c_str(), ios_base::app);
            if (!o.is_open()) {
                delete [] buf;
                buf = 0;
                return;
            }
            o.close();
        }
    }
    f = true;
}

void File::open(const char *fn, int fm) {
    open(string(fn), fm);
}

long File::tell() const {
    return p;
}

void File::seek(long pos) {
    if (!f)
        return;
    if (pos < 0)
        pos = 0;
    if (pos > length)
        pos = length;
    p = pos;
}

long File::size() const {
    return length;
}

bool File::eof() const {
    return p >= length;
}

void File::close() {
    if (!f)
        return;
    if (m) {
        ofstream out(fname.c_str());
        if (out.is_open()) {
            out.write(buf, length);
            out.close();
        }
    }
    delete [] buf;
    buf = 0;
    p = length = -1;
    m = f = false;
}

int File::getc() {
    if (mode & FIN) {
        return p >= length ? -1 : (buf[p++] & 0xff);
    } else
        return -1;
}

void File::putc(char c) {
    if (mode & FOUT) {
        ensureCapacity(1);
        buf[p++] = c;
        if (p > length)
            length = p;
        m = true;
    }
}

void File::puts(const char *s, int len) {
    if (mode & FOUT) {
        ensureCapacity(len);
        std::memcpy(buf + p, s, len);
        p += len;
        if (p > length)
            length = p;
        m = true;
    }
}

bool File::isOpen() const {
    return f;
}

void File::move(int off) {
    if (f)
        p += off;
}

bool File::readFile() {
    ifstream in(fname.c_str(), ios_base::binary | ios_base::in);
    long t = 0;
    int c;
    char *tb;
    length = 0;

    if (!in.is_open()) {
        return false;
    }
    while ((c = in.get()) != EOF) {
        if (t + 1 > capacity) {
            tb = buf;
            buf = new char[capacity <<= 1];
            std::memcpy(buf, tb, t);
            delete [] tb;
        }
        buf[t++] = c;
    }
    in.close();
    length = t;
    return true;
}

void File::ensureCapacity(int size) {
    while (p + size > capacity) {
        char *t = buf;
        buf = new char[capacity <<= 1];
        std::memcpy(buf, t, length);
        delete [] t;
    }
}

//--------------------------------
FM::~FM() {
    closeAll();
}

void FM::chDir(const char *s) {
    dir = s;
    if (!dir.empty() && dir.back() != '\\') {
        dir += '\\';
    }
}

bool FM::open(int num, const string &fn, int mode) {
    File &f = files[num];
    int fmode = File::FIN;
    switch (modes[num] = mode) {
    case APPEND:
        fmode = File::FAPP;
        break;
    case RANDOM:
        fmode = File::FRAN;
        break;
    case OUTPUT:
        fmode = File::FOUT;
        break;
    }
    f.open(dir + fn, fmode);
    return f.isOpen();
}

bool FM::isOpen(int num) const {
    return num >= 0 && num <= 2 ? files[num].isOpen() : false;
}

void FM::close(int num) {
    files[num].close();
}

int FM::mode(int num) const {
    return modes[num];
}

void FM::closeAll() {
    files[0].close();
    files[1].close();
    files[2].close();
}

long FM::tell(int num) {
    return files[num].tell();
}

void FM::seek(int num, long p) {
    files[num].seek(p);
}

bool FM::eof(int num) {
    return files[num].eof();
}

long FM::size(int num) {
    return files[num].size();
}

char FM::readByte(int num) {
    return files[num].getc();
}

void FM::writeByte(int num, char b) {
    files[num].putc(b);
}

void FM::writeReal(int num, double i) {
    static stringstream ss;

    ss.str("");
    ss.precision(9);
    ss << i;
    writeString(num, ss.str());
}

double FM::readReal(int num) {
    return std::strtod(getContent(num).c_str(), 0);
}

void FM::writeString(int num, const char *s) {
    files[num].puts(s, std::strlen(s));
}

void FM::writeString(int num, const string &s) {
    files[num].puts(s.c_str(), s.length());
}

string FM::readString(int num) { //È¥µôquotes
    File &fin = files[num];
    string s;
    int c = fin.getc();

    if (c == '"') {
        while ((c = fin.getc()) != '"' && c != -1) {
            s += c;
        }
    } else {
        do {
            s += c;
        } while ((c = fin.getc()) != ',' && c != 0xff && c != -1);
        if (c != -1)
            fin.move(-1);
    }
    return s;
}

string FM::readString(int num, int bytes) {
    File &fin = files[num];
    string s;
    int c, i = 0;

    while (i < bytes && (c = fin.getc()) != -1) {
        s += c;
        i++;
    }
    return s;
}

void FM::skip(int num) {
    files[num].move(1);
}

string FM::getContent(int num) { //±£Áôquotes
    File &fin = files[num];
    string s;
    int c = fin.getc();

    if (c == '"') {
        s += '"';
        while ((c = fin.getc()) != '"' && c != -1) {
            s += c;
        }
        if (c == '"')
            s += '"';
    } else {
        do {
            s += c;
        } while ((c = fin.getc()) != ',' && c != 0xff && c != -1);
        if (c != -1)
            fin.move(-1);
    }
    return s;
}