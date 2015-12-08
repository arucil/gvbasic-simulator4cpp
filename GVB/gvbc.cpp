#include <cmath>
#include <algorithm>
#include <sstream>
#include "gvb.h"
#include "error.h"

#include "condev.h"

GVB::GVB(Device *d) {
    l = 0;
    head = 0;
    loadFuncs();
    device = d;
    fileman.chDir("dat");
}

GVB::~GVB() {
    delete l;
    l = 0;
    head = 0;
    device = 0;
}

bool GVB::load(std::istream &in) {
    delete l;
    l = new Lexer(in);
    
    return true;
}

void GVB::peek() {
    try {
        tok = l->getToken();
    } catch (int) {
        cerror("Number format error");
    }
}

void GVB::match(int t) {
    if (t != tok)
        cerror("Token error: [%c], expected: [%c]", tok, t);
    peek();
}

void GVB::build() {
    //清理工作必须在gc之前进行
    clearVars();
    clearFuns();
    clearData();

    gc.gc();
    head = 0;
    line = 1;
    labels.clear();
    refs.clear();
    data.clear();
    label = 0;

    peek();
    S *cur = 0, *stmts;
    try {
        while (tok != -1) {
            stmts = aline(); //不可能为0
            if (tok != 10 && tok != -1)
                cerror("Token error: [%c], EOL / EOF expected", tok);
            peek();
            if (head == 0) {
                head = cur = stmts;
            } else if (stmts) {
                linkStmt(cur, stmts);
            }
            cur = getStmtTail(cur);
            line++;
        }
        solveRefs();
    } catch (...) {
        head = 0;
        throw;
    }
    delete l;
    l = 0;
}

void GVB::solveRefs() {//解决label引用
    Goto *g1;
    Gosub *g2;
    Restore *r;
    On *o;
    lbliter it;
    S *s;
    for (vector<S *>::iterator i = refs.begin(); i != refs.end(); i++) {
        s = *i;
        switch (s->type) {
        case S::GOTO:
            g1 = (Goto *) s;
            it = labels.find(g1->gotol);
            if (it == labels.end()) {
                line = g1->line;
                label = g1->label;
                cerror("Label not exist in GOTO: [%i]", g1->gotol);
            }
            g1->gotos = it->second;
            break;
        case S::GOSUB:
            g2 = (Gosub *) s;
            it = labels.find(g2->gotol);
            if (it == labels.end()) {
                line = g2->line;
                label = g2->label;
                cerror("Label not exist in GOSUB: [%i]", g2->gotol);
            }
            g2->gotos = it->second;
            break;
        case S::RESTORE:
            r = (Restore *) s;
            if (labels.find(r->rlabel) == labels.end()) {
                line = r->line;
                label = r->label;
                cerror("Label not exist in RESTORE: [%i]", r->rlabel);
            }
            break;
        case S::ON:
            o = (On *) s;
            for (int i = 0, j = o->labels.size(); i < j; i++) {
                it = labels.find(o->labels[i]);
                if (it == labels.end()) {
                    line = o->line;
                    label = o->label;
                    cerror("Label not exist in ON: [%i]", o->labels[i]);
                }
                o->stms[i] = it->second;
            }
            break;
        }
    }
}

S *GVB::aline() {
    label = l->ival;
    match(L::INT);
    if (labels.find(label) != labels.end())
        cerror("Label duplicate");
    data.addLabel(label);
    return labels[label] = stmts(); //记录一行的第一条语句
}

S *GVB::stmts() { //不可能为0
    S *head = 0, *cur, *stm;

    while (true) {
        stm = stmt(false);//可能为0
        if (stm == 0)
            break;
        if (head == 0) {
            head = cur = stm;
        } else {
            linkStmt(cur, stm);
        }
        cur = getStmtTail(cur);
        if (tok == 10 || tok == -1)
            break;
        match(':');
    }
    if (head == 0) {
        return gc.link(new None(line, label));
    }
    return head;
}

S *GVB::stmt(bool isInIf) {//可能返回stmts或0
    E *e1, *e2, *e3, *e4, *e5, *e6;
    S *s1, *s2, *s3;
    Print *p1;
    Id *a1, *a2;
    int i1, i2, i3;
    string s, f;
    Write *w;
    Read *r;
    On *o;
    Field *fl;

    while (true) {
        switch (tok) {
        case L::REM:
        case '\'': //哈哈哈哈
            l->skipTo(10);
            peek();
            return 0;
        case L::END:
            peek();
            return gc.link(new None(line, label, S::END));
        case L::CLS:
            peek();
            return gc.link(new None(line, label, S::CLS));
        case L::GRAPH:
            peek();
            return gc.link(new None(line, label, S::GRAPH));
        case L::TEXT:
            peek();
            return gc.link(new None(line, label, S::TEXT));
        case L::RETURN:
            peek();
            return gc.link(new None(line, label, S::RETURN));
        case L::POP:
            peek();
            return gc.link(new None(line, label, S::POP));
        case L::CLEAR:
            peek();
            return gc.link(new None(line, label, S::CLEAR));
        case L::INVERSE:
            peek();
            return gc.link(new None(line, label, S::INVERSE));
        case L::CONT:
            peek();
            continue;
        case L::BEEP:
            peek();
            continue;
        case L::PLAY:
            peek();
            expr(E::VSTRING);
            continue;
        case ':':
            peek();
            continue;
        case L::INKEY:
            peek();
            return gc.link(new None(line, label, S::INKEY));
        case L::LET:
            peek();
        case L::ID: //赋值
            a1 = getId();
            match('=');
            e1 = expr(a1->vtype & E::RVAL_MASK);
            return gc.link(new Assign(line, label, a1, e1));
            break;
        case L::CALL://call
            peek();
            e1 = expr(E::VREAL);
            return gc.link(new Call(line, label, e1));
        case L::GOTO:
            peek();
            i1 = l->ival;
            match(L::INT);
            return findlabel(i1, gc.link(new Goto(line, label, 0, i1)));
        case L::GOSUB:
            peek();
            i1 = l->ival;
            match(L::INT);
            return findlabel(i1, gc.link(new Gosub(line, label, 0, i1)));
        case L::IF:
            peek();
            e1 = expr(E::VREAL);
            s1 = s2 = 0;
            if (tok == L::GOTO) {
                peek();
                i1 = l->ival;
                match(L::INT);
                s1 = findlabel(i1, gc.link(new Goto(line, label, 0, i1)));
                if (tok == ':') {
                    peek();
                }
            } else if (tok == L::THEN) {
                peek();
                s1 = ifstmts();
            }
            if (tok == L::ELSE) {
                peek();
                s2 = ifstmts();
            }
            return gc.link(new If(line, label, e1, s1, s2));
        case L::DATA:
            while (true) {
                s.clear();
                l->skipSpace();
                if ((i1 = l->getc()) == '"') {
                    while ((i1 = l->getc()) != '"' && i1 != 13 && i1 != 10 && i1 != -1) {
                        s += i1;
                    }
                    if (i1 != '"') {
                        tok = i1 == 13 ? 10 : i1;
                    } else {
                        i1 = l->getc();
                    }
                } else {
                    while (i1 != ',' && i1 != 13 && i1 != ':' && i1 != 10
                        && i1 != -1) {
                        s += i1;
                        i1 = l->getc();
                    }
                }
                data.add(add0x1F(rtrim(s)));
                if (i1 != ',') {
                    tok = i1 == 13 ? 10 : i1;
                    break;
                }
            }
            continue;
        case L::DEF:
            peek();
            match(L::FN);
            f = l->sval;
            match(L::ID);
            match('(');
            s = l->sval;
            match(L::ID);
            match(')');
            match('=');
            i1 = getIdType(f);
            e1 = expr(i1 & E::RVAL_MASK);
            return gc.link(new DefFn(line, label, f, i1, s, getIdType(s), e1));
        case L::PRINT:
            peek();
            gc.link(p1 = new Print(line, label));
            while (tok != ':' && tok != 10 && tok != -1 && tok != L::ELSE) {
                if (tok != ';' && tok != ',') {
                    switch (tok) {
                    case L::TAB:
                        peek();
                        match('(');
                        e1 = expr(E::VREAL);
                        match(')');
                        p1->ps.push_back(gc.link(new Func(line, label, e1, Func::TAB,
                            E::VSTRING)));
                        break;
                    case L::SPC:
                        peek();
                        match('(');
                        e1 = expr(E::VREAL);
                        match(')');
                        p1->ps.push_back(gc.link(new Func(line, label, e1, Func::SPC,
                            E::VSTRING)));
                        break;
                    default:
                        p1->ps.push_back(expr());
                    }
                } else {
                        p1->ps.push_back(0);
                }
                p1->del.push_back(tok == ',' || tok == ':' || tok == 10 || tok == -1
                    || tok == L::ELSE ? Print::CR : Print::NO);
                if (tok == ';' || tok == ',')
                    peek();
            }
            if (p1->ps.empty()) {
                p1->ps.push_back(0);
                p1->del.push_back(Print::CR);
            }
            return p1;
        case L::FOR:
            peek();
            s = l->sval;
            match(L::ID);
            if ((i1 = getIdType(s)) == E::VSTRING) {
                cerror("Incompatible Id type in FOR: [%s]", s);
            }
            match('=');
            e1 = expr(E::VREAL);
            a1 = gc.link(new Id(line, label, s, i1));
            s1 = gc.link(new Assign(line, label, a1, e1));
            match(L::TO);
            e1 = expr(E::VREAL);
            if (tok == L::STEP) {
                peek();
                e2 = expr(E::VREAL);
            } else {
                e2 = 0;
            }
            s2 = gc.link(new For(line, label, s, e1, e2));
            s1->next = s2;
            return s1;
        case L::NEXT:
            peek();
            if (tok != L::ID) {
                return gc.link(new Next(line, label));
            }
            s1 = 0;
            while (true) {
                s = l->sval;
                match(L::ID);
                if (getIdType(s) == E::VSTRING) {
                    cerror("Incompatible Id type in NEXT: [%s]", s);
                }
                s3 = gc.link(new Next(line, label, s));
                if (s1 == 0) {
                    s1 = s2 = s3;
                } else {
                    s2->next = s3;
                    s2 = s3;
                }
                if (tok != ',')
                    break;
                peek();
            }
            return s1;
        case L::DIM:
            peek();
            s1 = 0;
            while (true) {
                a1 = getId();
                s3 = gc.link(new Dim(line, label, a1));
                if (s1 == 0) {
                    s1 = s2 = s3;
                } else {
                    s2->next = s3;
                    s2 = s3;
                }
                if (tok != ',')
                    break;
                peek();
            }
            return s1;
        case L::WHILE:
            peek();
            e1 = expr(E::VREAL);
            return gc.link(new While(line, label, e1));
        case L::WEND:
            peek();
            return gc.link(new None(line, label, S::WEND));
        case L::INPUT:
            peek();
            if (tok == '#' || tok == L::INT) { //file input
                i1 = getFileNum();
                match(',');
                FInput *fi = (FInput *) gc.link(new FInput(line, label, i1));
                while (true) {
                    a1 = getId();
                    fi->ids.push_back(a1);
                    if (tok != ',')
                        break;
                    peek();
                }
                return fi;
            } else { //key input
                if (tok == L::STRING) {
                    s = l->sval;
                    peek();
                    match(';');
                } else
                    s.clear();
                Input *i = (Input *) gc.link(new Input(line, label, s));
                while (true) {
                    a1 = getId();
                    i->ids.push_back(a1);
                    if (tok != ',')
                        break;
                    peek();
                }
                return i;
            }
        case L::WRITE:
            peek();
            i1 = getFileNum();
            match(',');
            gc.link(w = new Write(line, label, i1));
            while (true) {
                e1 = expr();
                w->es.push_back(e1);
                if (tok != ',')
                    break;
                peek();
            }
            return w;
        case L::READ:
            peek();
            gc.link(r = new Read(line, label));
            while (true) {
                a1 = getId();
                r->ids.push_back(a1);
                if (tok != ',')
                    break;
                peek();
            }
            return r;
        case L::CLOSE:
            peek();
            i1 = getFileNum();
            return gc.link(new Close(line, label, i1));
        case L::OPEN:
            peek();
            e1 = expr(E::VSTRING);
            match(L::FOR);
            switch (tok) {
            case L::INPUT:
                i1 = Open::MINPUT;
                peek();
                break;
            case L::ID:
                s = l->sval;
                if (s.compare("output") == 0) {
                    i1 = Open::MOUTPUT;
                    peek();
                } else if (s.compare("random") == 0) {
                    i1 = Open::MRANDOM;
                    peek();
                } else if (s.compare("outputas") == 0) {
                    i1 = Open::MOUTPUT;
                    tok = L::AS;
                } else if (s.compare("randomas") == 0) {
                    i1 = Open::MRANDOM;
                    tok = L::AS;
                } else if (s.compare("inputas") == 0) {
                    i1 = Open::MINPUT;
                    tok = L::AS;
                } else if (s.compare("append") == 0) {
                    i1 = Open::MAPPEND;
                    peek();
                } else if (s.compare("appendas") == 0) {
                    i1 = Open::MAPPEND;
                    tok = L::AS;
                } else if (s.compare("binary") == 0) {
                    i1 = Open::MBINARY;
                    peek();
                } else {
                    cerror("File mode error: [%s]", s);
                }
                break;
            default:
                cerror("Not file mode: [%c]", tok);
            }
            match(L::AS);
            i2 = getFileNum();
            if (i1 == Open::MRANDOM && tok == L::ID && l->sval.compare("len") == 0) {
                peek();
                match('=');
                i3 = l->ival;
                match(L::INT);
            } else
                i3 = Open::NOLEN;
            return gc.link(new Open(line, label, i2, e1, i1, i3));
        case L::LOCATE:
            peek();
            if (tok == ',')
                e1 = 0;
            else
                e1 = expr(E::VREAL); //row
            match(',');
            e2 = expr(E::VREAL); //col
            return gc.link(new Locate(line, label, e1, e2));
        case L::POKE:
            peek();
            e1 = expr(E::VREAL);
            match(',');
            e2 = expr(E::VREAL);
            return gc.link(new Poke(line, label, e1, e2));
        case L::SWAP:
            peek();
            a1 = getId();
            match(',');
            a2 = getId();
            if (a1->vtype != a2->vtype) {
                cerror("Incompatible type in SWAP: [%t, %t]",
                    a1->vtype, a2->vtype);
            }
            return gc.link(new Swap(line, label, a1, a2));
        case L::RESTORE:
            peek();
            if (tok == L::INT) {
                i1 = l->ival;
                peek();
            } else
                i1 = Restore::NOLABEL;
            return findlabel(i1, (Restore *) gc.link(new Restore(line, label, i1)));
        case L::ON:
            peek();
            e1 = expr(E::VREAL);
            if (tok == L::GOTO) {
                i1 = 0;
            } else if (tok == L::GOSUB) {
                i1 = 1;
            } else {
                cerror("Token error: [%c], GOTO / GOSUB expected", tok);
            }
            peek();
            o = (On *) gc.link(new On(line, label, e1, i1));
            while (true) {
                i1 = l->ival;
                match(L::INT);
                o->labels.push_back(i1);
                if (tok != ',')
                    break;
                peek();
            }
            return findlabel(o);
        case L::DRAW:
            peek();
            e1 = expr(E::VREAL);
            match(',');
            e2 = expr(E::VREAL);
            if (tok == ',') {
                peek();
                e3 = expr(E::VREAL);
            } else
                e3 = 0;
            return gc.link(new Draw(line, label, e1, e2, e3));
        case L::LINE:
            peek();
            e1 = expr(E::VREAL);
            match(',');
            e2 = expr(E::VREAL);
            match(',');
            e3 = expr(E::VREAL);
            match(',');
            e4 = expr(E::VREAL);
            if (tok == ',') {
                peek();
                e5 = expr(E::VREAL);
            } else
                e5 = 0;
            return gc.link(new Line(line, label, e1, e2, e3, e4, e5));
        case L::BOX:
            peek();
            e1 = expr(E::VREAL);
            match(',');
            e2 = expr(E::VREAL);
            match(',');
            e3 = expr(E::VREAL);
            match(',');
            e4 = expr(E::VREAL);
            if (tok == ',') {
                peek();
                e5 = expr(E::VREAL);
                if (tok == ',') {
                    peek();
                    e6 = expr(E::VREAL);
                } else
                    e6 = 0;
            } else
                e5 = e6 = 0;
            return gc.link(new Box(line, label, e1, e2, e3, e4, e5, e6));
        case L::CIRCLE:
            peek();
            e1 = expr(E::VREAL);
            match(',');
            e2 = expr(E::VREAL);
            match(',');
            e3 = expr(E::VREAL);
            if (tok == ',') {
                peek();
                e4 = expr(E::VREAL);
                if (tok == ',') {
                    peek();
                    e5 = expr(E::VREAL);
                } else
                    e5 = 0;
            } else
                e5 = e4 = 0;
            return gc.link(new Circle(line, label, e1, e2, e3, e4, e5));
        case L::ELLIPSE:
            peek();
            e1 = expr(E::VREAL);
            match(',');
            e2 = expr(E::VREAL);
            match(',');
            e3 = expr(E::VREAL);
            match(',');
            e4 = expr(E::VREAL);
            if (tok == ',') {
                peek();
                e5 = expr(E::VREAL);
                if (tok == ',') {
                    peek();
                    e6 = expr(E::VREAL);
                } else
                    e6 = 0;
            } else
                e5 = e6 = 0;
            return gc.link(new Ellipse(line, label, e1, e2, e3, e4, e5, e6));
        case L::LSET:
            peek();
            a1 = getId();
            if (a1->vtype != E::VSTRING) {
                cerror("Need string Id in LSET: [%s]", a1->id);
            }
            match('=');
            e1 = expr(E::VSTRING);
            return gc.link(new Lset(line, label, a1, e1));
        case L::RSET:
            peek();
            a1 = getId();
            if (a1->vtype != E::VSTRING) {
                cerror("Need string Id in RSET: [%s]", a1->id);
            }
            match('=');
            e1 = expr(E::VSTRING);
            return gc.link(new Rset(line, label, a1, e1));
        case L::PUT:
            peek();
            i1 = getFileNum();
            match(',');
            e1 = expr(E::VREAL);
            return gc.link(new Put(line, label, i1, e1));
        case L::GET:
            peek();
            i1 = getFileNum();
            match(',');
            e1 = expr(E::VREAL);
            return gc.link(new Get(line, label, i1, e1));
        case L::FIELD:
            peek();
            i1 = getFileNum();
            match(',');
            fl = (Field *) gc.link(new Field(line, label, i1));
            i2 = 0;
            while (true) {
                i1 = l->ival;
                match(L::INT);
                match(L::AS);
                s = l->sval;
                match(L::ID);
                if (getIdType(s) != E::VSTRING) {
                    cerror("Need string Id in FIELD: [%s]", s);
                }
                fl->bys.push_back(i1);
                i2 += i1;
                fl->ids.push_back(s);
                if (tok != ',')
                    break;
                peek();
            }
            fl->total = i2;
            return fl;
        case L::INT:
            if (isInIf) {
                i1 = l->ival;
                peek();
                return findlabel(i1, gc.link(new Goto(line, label, 0, i1)));
            }
            cerror("Token error: [%c], statement expected", tok);
            //自定义
        case L::SLEEP:
            peek();
            e1 = expr(E::VREAL);
            return gc.link(new SSleep(line, label, e1));
        case L::PAINT:
            peek();
            e1 = expr(E::VREAL);
            match(',');
            e2 = expr(E::VREAL);
            match(',');
            e3 = expr(E::VREAL);
            match(',');
            e4 = expr(E::VREAL);
            match(',');
            e5 = expr(E::VREAL);
            if (tok == ',') {
                peek();
                e6 = expr(E::VREAL);
            } else
                e6 = 0;
            return gc.link(new SPaint(line, label, e1, e2, e3, e4, e5, e6));
        case L::LOAD:
            peek();
            e1 = expr(E::VREAL);
            match(',');
            s.clear();
            while (true) {
                if (tok != L::INT) {
                    cerror("Token error: [%c], integer literal expected", tok);
                }
                if (l->ival > 255) {
                    cerror("Integer overflow: %i", (int) l->ival);
                }
                s.push_back(l->ival);
                peek();
                if (tok == ',') {
                    peek();
                } else {
                    break;
                }
            }
            return gc.link(new SLoad(line, label, e1, s));
        case L::FSEEK:
            peek();
            i1 = getFileNum();
            match(',');
            e1 = expr(E::VREAL);
            return gc.link(new SFseek(line, label, i1, e1));
        case L::FPUTC:
            peek();
            i1 = getFileNum();
            match(',');
            e1 = expr(E::VSTRING);
            return gc.link(new SFputc(line, label, i1, e1));
        case L::FREAD:
            peek();
            i1 = getFileNum();
            match(',');
            e1 = expr(E::VREAL);
            match(',');
            e2 = expr(E::VREAL);
            return gc.link(new SFread(line, label, i1, e1, e2));
        case L::FWRITE:
            peek();
            i1 = getFileNum();
            match(',');
            e1 = expr(E::VREAL);
            match(',');
            e2 = expr(E::VREAL);
            return gc.link(new SFwrite(line, label, i1, e1, e2));

        default://eol eof else ...
            return 0;
        }
    }
}

int GVB::getFileNum() {
    if (tok == '#')
        peek();
    int i = l->ival - 1;
    match(L::INT);
    if (i > 2 || i < 0) {
       cerror("File number error: [%i]", i + 1);
    }
    return i;
}

S *GVB::findlabel(int lb, Goto *s) {
    lbliter i = labels.find(lb);
    if (i != labels.end()) {
        s->gotos = i->second;
    } else {
        refs.push_back(s);
    }
    return s;
}

S *GVB::findlabel(int lb, Gosub *s) {
    lbliter i = labels.find(lb);
    if (i != labels.end()) {
        s->gotos = i->second;
    } else {
        refs.push_back(s);
    }
    return s;
}

S *GVB::findlabel(int lb, Restore *s) {
    if (lb == Restore::NOLABEL)
        return s;
    if (labels.find(lb) == labels.end()) {
        refs.push_back(s);
    }
    return s;
}

S *GVB::findlabel(On *s) {
    bool f = true;
    for (int i = 0, j = s->labels.size(); i < j; i++) {
        lbliter it = labels.find(s->labels[i]);
        if (it != labels.end()) {
            s->stms.push_back(it->second);
        } else {
            s->stms.push_back(0);
            if (f) {
                refs.push_back(s);
                f = false;
            }
        }
    }
    return s;
}

S *GVB::getStmtTail(S *s) {
    if (s == 0) {
        return 0;
    } else {
        while (s->next)
            s = s->next;
        return s;
    }
}

S *GVB::ifstmts() {
    S *head = 0, *cur, *stm;

    while (true) {
        stm = stmt(true);//可能为0
        if (stm == 0)
            break;
        if (head == 0) {
            head = cur = stm;
        } else {
            linkStmt(cur, stm);
        }
        cur = getStmtTail(cur);
        if (tok == 10 || tok == L::ELSE || tok == -1)
            break;
        match(':');
    }
    if (head == 0) {
        return gc.link(new None(line, label));
    }
    return head;
}

void GVB::linkStmt(S *s, S *next) {
    If *i;
    if (s) {
        s->next = next;
        if (s->type == S::IF) {
            i = (If *) s;
            linkStmt(getStmtTail(i->stm), next);
            linkStmt(getStmtTail(i->elsestm), next);
        }
    }
}
#if 0
S *GVB::ifstmt() {
    int l1;

    switch (tok) {
    case L::INT:
        l1 = l->ival;
        peek();
        return findlabel(l1, gc.link(new Goto(line, label, 0, l1)));
    default:
        return stmt();
    }
}
#endif
E*GVB::expr(int vtype) {
    E *e1 = E_(0);
    if (e1->vtype != vtype) {
        cerror("Rvalue type error: [%t], [%t] expected", e1->vtype, vtype);
    }
    return e1;
}

E *GVB::expr() {
    return E_(0);
}

E *GVB::E_(int p) {
    E *e1 = fold(F()), *e2;
    int i, pp;
    while ((pp = getp(tok)) > p) {
        i = tok;
        peek();
        e2 = E_(pp);

        if (e1->vtype != e2->vtype) {
            cerror("Incompatible operand type in binary: [%t, %t]",
                e1->vtype, e2->vtype);
        }
        switch (i) {
        case '-': case '*': case '/': case '^': case L::AND: case L::OR:
            if (e1->vtype != E::VREAL) {
                cerror("Rvalue type error in binary: [%t], [%t] expected",
                    e1->vtype, E::VREAL);
            }
        }
        e1 = fold(gc.link(new Binary(line, label, i, e1, e2, i == '+' ? e1->vtype : E::VREAL)));
    }
    return e1;
}

int GVB::getp(int op) const {
    switch (op) {
    case L::GE: case L::LE: case L::NEQ: case '=': case '>': case '<':
        return PRI_REL;
    case '+': case '-':
        return PRI_ADD_MIN;
    case '*': case '/':
        return PRI_TIM_DIV;
    case '^':
        return PRI_POW;
    case L::AND: case L::OR:
        return PRI_LOG;
    default:
        return 0;
    }
}

E *GVB::F() {
    long i1, i2;
    double r1;
    string s1;
    E *e1, *e2, *e3;
    fniter it;
    Id *a1;

    switch (tok) {
    case L::INT:
        i1 = l->ival;
        peek();
        return gc.link(new Real(line, label, i1));
    case L::REAL:
        r1 = l->rval;
        peek();
        return gc.link(new Real(line, label, r1));
    case L::STRING:
        s1 = add0x1F(l->sval); //汉字加上0x1f前缀
        peek();
        return gc.link(new Str(line, label, s1));
    case '+': case '-':
        i1 = tok;
        peek();
        e1 = E_(PRI_NEG);
        if (e1->vtype != E::VREAL) {
            cerror("Rvalue type error in unary: [%t], [%t] expected", e1->vtype, E::VREAL);
        }
        return i1 == '-' ? gc.link(new Func(line, label, e1, Func::NEG, E::VREAL)) : e1;
    case L::NOT:
        peek();
        e1 = E_(PRI_NOT);
        if (e1->vtype != E::VREAL) {
            cerror("Rvalue type error in NOT: [%t], [%t] expected", e1->vtype, E::VREAL);
        }
        return gc.link(new Func(line, label, e1, Func::NOT, E::VREAL));
    case L::INKEY://////////////////////
        peek();
        return gc.link(new Inkey(line, label));
    case '(':
        peek();
        e1 = expr();
        match(')');
        return e1;
    case L::FN:
        peek();
        s1 = l->sval;
        match(L::ID);
        match('(');
        e1 = expr();
        match(')');
        return gc.link(new CallFn(line, label, e1, s1, getIdType(s1) & E::RVAL_MASK));
    case L::ID:
        it = funcs.find(l->sval);
        if (it != funcs.end()) { //是函数
            i1 = it->second;
            i2 = Func::getFuncType(i1);
            peek();
            match('(');
            e1 = expr(Func::getFuncParamType(i1, 0));
            switch (i1) {
            case Func::LEFT: case Func::RIGHT: case Func::POINT: ///////////
                match(',');
                e2 = expr(E::VREAL);
                break;
            case Func::MID:
                match(',');
                e2 = expr(E::VREAL);
                if (tok == ',') {
                    peek();
                    e3 = expr(E::VREAL);
                } else {
                    e3 = 0;
                }
            }
            match(')');
            switch (i1) {
            case Func::LEFT: case Func::RIGHT: case Func::POINT: ///////////
                return gc.link(new Func(line, label, e1, e2, i1, i2));
            case Func::MID:
                return gc.link(new Func(line, label, e1, e2, e3, i1, i2));
            default:
                return gc.link(new Func(line, label, e1, i1, i2));
            }
        } else { //是id
            a1 = getId();
            a1->vtype &= E::RVAL_MASK;
            return a1;
        }
    default:
        cerror("Token error: [%c], expression expected", tok);
        return 0;
    }
}

int GVB::getIdType(const string &s) const {
    switch (s.back()) {
    case '$':
        return E::VSTRING;
    case '%':
        return E::VINT;
    default:
        return E::VREAL;
    }
}

Id *GVB::getId() {
    string id = l->sval;

    match(L::ID);
    int vt = getIdType(id);
    if (tok == '(') {
        peek();
        vector<E *> v;
        while (true) {
            v.push_back(expr(E::VREAL));
            if (tok == ')') {
                peek();
                break;
            }
            match(',');
        }
        return gc.link(new Access(line, label, id, v, vt));
    } else {
        return gc.link(new Id(line, label, id, vt));
    }
}

E *GVB::fold(E *e) { //常量折叠
    Binary *e1;
    Func *e2;
    E *t = e;
    double r1;

    switch (e->type) {
    case E::BINARY:
        e1 = (Binary *) t;
        if ((e1->l->type & E::CON_MASK) && (e1->r->type & E::CON_MASK)) {
            switch (e1->op) {
            case '+':
                if (e1->l->vtype == E::VSTRING) {
                    e = gc.link(new Str(line, label, ((Str *) e1->l)->sval
                        + ((Str *) e1->r)->sval));
                } else {
                    e = gc.link(new Real(line, label, ((Real *) e1->l)->rval
                        + ((Real *) e1->r)->rval));
                }
                break;
            case '-':
                e = gc.link(new Real(line, label, ((Real *) e1->l)->rval
                    - ((Real *) e1->r)->rval));
                break;
            case '*':
                e = gc.link(new Real(line, label, ((Real *) e1->l)->rval
                    * ((Real *) e1->r)->rval));
                break;
            case '/':
                r1 = ((Real *) e1->r)->rval;
                if (r1 == 0.0) {
                    cerror("Division by zero");
                }
                e = gc.link(new Real(line, label, ((Real *) e1->l)->rval / r1));
                break;
            case '^':
                e = gc.link(new Real(line, label, std::pow(((Real *) e1->l)->rval
                    , ((Real *) e1->r)->rval)));
                break;
            case '=':
                if (e1->l->vtype == E::VSTRING) {
                    e = gc.link(new Real(line, label, ((Str *) e1->l)->sval.
                        compare(((Str *) e1->r)->sval) == 0));
                } else {
                    e = gc.link(new Real(line, label, isZero(((Real *) e1->l)->rval
                        - ((Real *) e1->r)->rval)));
                }
                break;
            case L::NEQ:
                if (e1->l->vtype == E::VSTRING) {
                    e = gc.link(new Real(line, label, ((Str *) e1->l)->sval.
                        compare(((Str *) e1->r)->sval) != 0));
                } else {
                    e = gc.link(new Real(line, label, notZero(((Real *) e1->l)->rval
                        - ((Real *) e1->r)->rval)));
                }
                break;
            }
            if (e != t) {
                gc.remove(e1->l);
                gc.remove(e1->r);
                gc.remove(t);
            }
        }
        break;
    case E::FUNC:
        e2 = (Func *) e;
        if (e2->x->type & E::CON_MASK) {
            switch (e2->f) {
            case Func::NEG:
                e = gc.link(new Real(line, label, -((Real *) e2->x)->rval));
                break;
            case Func::NOT:
                e = gc.link(new Real(line, label, !((Real *) e2->x)->rval));
                break;
            }
            if (e != t) {
                gc.remove(e2->x);
                gc.remove(t);
            }
        }
    }
    return e;
}

bool GVB::isZero(double r) const {
    return std::fabs(r) < 1e-14;
}

bool GVB::notZero(double r) const {
    return std::fabs(r) >= 1e-14;
}

string &GVB::add0x1F(string &s) const {
    for (int i = 0, j = s.length(); i < j;) {
        if (s[i++] & 128) {
            if (i < j) {
                s.insert(i - 1, 1, 0x1f);
                j++, i += 2;
            }
        }
    }
    return s;
}

string &GVB::remove0x1F(string &s) const {
    int c = 0, j = s.length();
    for (int i = 0; i < j; i++) {
        if (s[i] == 0x1f) {
            c++;
        } else if (c) {
            s[i - c] = s[i];
        }
    }
    s.erase(j - c, c);
    return s;
}

string &GVB::removeNull(string &s) const {
    int c = 0, j = s.length();
    for (int i = 0; i < j; i++) {
        if (!s[i]) {
            c++;
        } else if (c) {
            s[i - c] = s[i];
        }
    }
    s.erase(j - c, c);
    return s;
}

string &GVB::rtrim(string &s) const {
    int t = 0, i;
    for (i = s.length() - 1; i >= 0; i--) {
        if (s[i] == ' ')
            t++;
        else
            break;
    }
    return s.erase(i + 1, t);
}

void GVB::loadFuncs() {
    funcs["abs"] = Func::ABS; funcs["asc"] = Func::ASC; funcs["atn"] = Func::ATN;
    funcs["chr$"] = Func::CHR; funcs["cos"] = Func::COS; funcs["cvi$"] = Func::CVI;
    funcs["cvs$"] = Func::CVS; funcs["exp"] = Func::EXP; funcs["eof"] = Func::FEOF;
    funcs["int"] = Func::INT; funcs["left$"] = Func::LEFT; funcs["len"] = Func::LEN;
    funcs["lof"] = Func::LOF; funcs["log"] = Func::LOG; funcs["mid$"] = Func::MID;
    funcs["mki$"] = Func::MKI; funcs["mks$"] = Func::MKS; funcs["peek"] = Func::PEEK;
    funcs["pos"] = Func::POS; funcs["right$"] = Func::RIGHT; funcs["rnd"] = Func::RND;
    funcs["sgn"] = Func::SGN; funcs["sin"] = Func::SIN; funcs["sqr"] = Func::SQR;
    funcs["str$"] = Func::STR; funcs["tan"] = Func::TAN; funcs["val"] = Func::VAL;

    funcs["point"] = Func::POINT; funcs["checkkey"] = Func::CHECKKEY;
    funcs["fopen"] = Func::FOPEN; funcs["fgetc"] = Func::FGETC;
    funcs["ftell"] = Func::FTELL;
}

void GVB::cerror(const char *s, ...) const {
    std::stringstream str;
    va_list a;
    int t;

    str.precision(9);
    va_start(a, s);
    while (*s) {
        if (*s == '%') {
            switch (*++s) {
            case '%':
                str.put('%');
                break;
            case 'c': //token
                t = va_arg(a, int);
                if (t > 32 && t < 128)
                    str << (char) t;
                else
                    str << t;
                break;
            case 's': //string
                {
                    string s = va_arg(a, string);
                    if (s.length() > 30) {
                        s.erase(30, s.length());
                    }
                    str << s;
                }
                break;
            case 'S':
                str << va_arg(a, const char *);
                break;
            case 't': //value type
                str << E::typeDesc(va_arg(a, int));
                break;
            case 'i': //integer
                str << va_arg(a, int);
                break;
            case 'f': //double
                str << va_arg(a, double);
                break;
            default:
                s--;
            }
        } else
            str.put(*s);
        s++;
    }
    va_end(a);

    throw CE(line, label, str.str());
}

void Data::restore() {
    p = 0;
}

void Data::restore(int label) {
    p = labels.find(label)->second;
}

void Data::add(const string &s) {
    data.push_back(s);
}

void Data::addLabel(int lb) {
    labels[lb] = data.size();
}

string &Data::get() {
    return data[p++];
}

int Data::size() const {
    return data.size();
}

bool Data::end() const {
    return p >= data.size();
}

void Data::clear() {
    labels.clear();
    data.clear();
    p = 0;
}