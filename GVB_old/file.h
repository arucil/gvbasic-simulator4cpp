#pragma once

#include <string>

using std::string;

class File {
    static const int DEFAULT_BUFFER_SIZE = 1024;
    char *buf;
    int mode;
    long p, capacity, length;
    string fname;
    bool f, m;

    bool readFile();
    void ensureCapacity(int);
public:
    static const int FIN = 0x1, FOUT = 0x2, FEND = 0x4, FRAN = FIN | FOUT, FAPP = FOUT | FEND;

    ~File();
    File() : buf(0), p(-1), length(-1), f(false), m(false) {}
    void open(const char *, int);
    void open(const string &, int);
    long tell() const;
    void seek(long);
    long size() const;
    bool eof() const;
    void close();
    int getc();
    void putc(char);
    void puts(const char *, int);
    bool isOpen() const;
    void move(int off);
};


class FileManager {
    File files[3];
    int modes[3];
    string dir;
    
    static const int READ = 0x80, WRITE = 0x40;

    string getContent(int);
public:
    static const int APPEND = WRITE | 1, INPUT = READ, OUTPUT = WRITE,
        RANDOM = READ | WRITE;

    void chDir(const char *);
    bool open(int, const string &, int mode);
    bool isOpen(int) const;
    int mode(int) const;
    void close(int);
    void closeAll();
    long tell(int);
    void seek(int, long);
    bool eof(int);
    long size(int);
    void skip(int);
    char readByte(int); //-1 -> FF
    void writeByte(int, char);
    //long readInt(int);
    double readReal(int);
    void writeReal(int, double);
    string readString(int);
    string readString(int, int bytes);
    void writeString(int, const string &);
    void writeString(int, const char *);
    //bool error(int);
    //void clearError(int);
    ~FileManager();
};

typedef FileManager FM;