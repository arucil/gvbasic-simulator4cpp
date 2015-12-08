#include "lex.h"
#include "gvb.h"
#include "error.h"
#include "condev.h"
#include "debug.h"
#include <fstream>
#include <iostream>

using namespace std;

bool terminated() {return false;}

void delay() {}

void testlex(istream &in,ostream&out) {
    
    Lexer l(in);

    int tok;l.skipTo(10);out<<"skip:"<<l.sval<<endl;
    do {
        tok = l.getToken();
        switch (tok) {
        case L::GE:out<<">=";break;
        case L::LE:out<<"<=";break;
        case L::NEQ:out<<"<>";break;
        case L::ID:out<<"id:"<<l.sval;break;
        case L::STRING:out<<"string:\""<<l.sval<<'"';break;
        case L::INT:out<<"int:"<<l.ival;break;
        case L::REAL:out<<"real:"<<l.rval;break;
        default:
            if (tok > 32 && tok < 128)
                out<<(char)tok;
            else
                out<<tok;
        }
        out<<endl;
    } while (tok != -1);
}

void test(ostream &out) {
    {
        Device *dd = new Device0;
        GVB p(dd);
        ifstream fin;
        try{
        fin.open("1.txt");

        
        p.load(fin);
        p.build();fin.close();
        S *c=p.getTree();
        show(c, out);out<<endl;
        out<<"--------------------------------------output:\n";
        p.execute();
        out<<endl;
    }catch(CE &e){out<<e.description()<<endl;}catch(exception e){out<<e.what();}
    show(p, out);
    out<<"nodes: "<<O::n<<" -> ";
    delete dd;
    }
    out<<O::n;
}

int main() {
    try {
        //ofstream fout("2.txt");
        test(cout);
        //fout.close();
    } catch (exception &e) {
        cout <<e.what();
    }

    /*FM f;
    f.open(0, "1.dat", FM::INPUT);
    f.open(1,"2.dat",FM::RANDOM);
    string s;
    while (!f.eof(0)) {
        cout<<(s=f.readString(0))<<12345;f.skip(0);
        f.writeByte(1,'"');f.writeString(1,s);f.writeByte(1,'"');f.writeByte(1,0xff);
    }f.seek(1,100);
    f.writeReal(1,1234.123);
    
    cout<<f.readReal(1)<<" "<<f.size(1);*/

    cin.sync();
    cin.get();
}