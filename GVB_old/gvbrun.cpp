#include <cmath>
#include <sstream>
#include <ctime>
#include "gvb.h"

using namespace std; //ok

typedef Func F;

size_t IdHash::operator()(const IdKey &v) const {
    int l = v.id.length();
    long h = 0;
    for (int i = 0; i < l; i++)
        h = h * -1664117991L + (v.id[i] & 0xff);
    h = h * 31 + v.type;
    return h;
}

bool IdHash::operator()(const IdKey &v1, const IdKey &v2) const {
    return v1.id.compare(v2.id) < 0;
}

void GVB::setDevice(Device *d) {
    device = d;
}

void GVB::execute() {
    if (!device)
        return;

    loops.clear();
    subs.clear();
    clearVars();
    clearFuns();
    clearData();
    clearFiles();
    randomize(time(0));
    data.restore();
    
    device->setMode(Device::TEXT);
    try {
        traverse(head);
    } catch (int) { //terminated
    }

    clearFiles();
    /*loops.clear();
    subs.clear();
    clearData();
    clearFuns();
    clearVars();*///测试用
}

void GVB::clearVars() {
    for (variter i = vars.begin(); i != vars.end(); i++) {
        gc.remove(i->second);
    }
    vars.clear();
}

void GVB::clearFuns() {
    for (funiter i = funs.begin(); i != funs.end(); i++) {
        gc.remove(i->second);
    }
    funs.clear();
}

void GVB::clearFiles() {
    fileman.closeAll();
}

void GVB::pop() {
    V *t = dstack.back();
    dstack.pop_back();
    switch (top.vtype = t->vtype) {
    case E::VINT:
        top.ival = t->ival; break;
    case E::VREAL:
        top.rval = t->rval; break;
    case E::VSTRING:
        top.sval = t->sval;
    }
    gc.remove(t);
}

void GVB::push(const V *v) { //仅用于ID
    dstack.push_back(gc.link(new V(v)));
}

void GVB::push(double r) {
    dstack.push_back(gc.link(new V(r)));
}

void GVB::push(const string &s) {
    dstack.push_back(gc.link(new V(s)));
}

bool GVB::empty() const {
    return dstack.empty();
}

void GVB::clearData() {
    while (!dstack.empty()) {
        gc.remove(dstack.back());
        dstack.pop_back();
    }
}

void GVB::traverse(S *s) {
    Dim *d1;
    Assign *as1;
    Id *id1;
    Access *ac1;
    For *f1;
    Next *n1;
    Print *p1;
    If *if1;
    On *o1;
    DefFn *df;
    Locate *lc;
    Swap *sw;
    Read *rd;
    Lset *ls;
    Rset *rs;
    Open *op;
    Write *wr;
    FInput *fip;
    Input *ip;
    Field *fl;
    Put *fp;
    Get *fg;
    Draw *pd;
    Line *pl;
    Box *pb;
    Ellipse *pe;
    Circle *pc;
    S *t1;
    V *v1, *v2;
    Array *a1, *a2;
    IdKey ik;
    variter vi;
    stmiter si;
    int i0, i1, i2, i3, i4, i;
    long l1, l2, l3;
    double r1, r2;
    string s1;
    bool b1;
    E *e1;
    Func *f2;
    stringstream ss;

    while (s) {
        if (terminated()) {
            throw 1;
        }
        line = s->line;
        label = s->label;
        switch (s->type) {
        case S::CLEAR:
            loops.clear();
            subs.clear();
            clearVars();
            clearFuns();
            clearData();
            clearFiles();
            data.restore();
        case S::NONE: case S::INVERSE:
            break;
        case S::END:
            s = 0;
            continue;
        case S::INKEY:
            device->getkey();
            break;
        case S::CLS:
            device->cls();
            break;
        case S::GRAPH:
            device->setMode(Device::GRAPH);
            break;
        case S::TEXT:
            device->setMode(Device::TEXT);
            break;
        case S::DIM:
            d1 = (Dim *) s;
            id1 = d1->id;
            ik.id = id1->id;
            switch (id1->type) {
            case E::ID:
                ik.type = V::ID;
                vi = vars.find(ik);
                if (vi != vars.end()) {
                    rerror("Redefine variable: %s", ik.id);
                }
                switch (id1->vtype) {
                case E::VINT:
                    v1 = gc.link(new V(0L));
                    break;
                case E::VREAL:
                    v1 = gc.link(new V(0.0));
                    break;
                case E::VSTRING:
                    v1 = gc.link(new V(""));
                    break;
                default:
                    rerror("Id type error in DIM: %s, [%t]", ik.id, id1->vtype);
                }
                vars[ik] = v1;
                break;
            case E::ACCESS:
                ac1 = (Access *) id1;
                ik.type = V::ARRAY;
                vi = vars.find(ik);
                if (vi != vars.end()) {
                    rerror("Redefine array: %s", ik.id);
                }
                a1 = gc.link(new Array);
                i1 = ac1->index.size();
                l1 = 1;
                for (i = 0; i < i1; i++) {
                    eval(ac1->index[i]);
                    i2 = top.rval;
                    if (i2 < 0) {
                        rerror("Bad index in DIM array: %s, [%i]", ik.id, i2);
                    }
                    a1->bounds.push_back(++i2);
                    l1 *= i2;
                }
                switch (a1->vtype = id1->vtype) {
                case E::VINT:
                    for (i = 0; i < l1; i++) {
                        a1->ivals.push_back(0L);
                    }
                    break;
                case E::VREAL:
                    for (i = 0; i < l1; i++) {
                        a1->rvals.push_back(0.0);
                    }
                    break;
                case E::VSTRING:
                    for (i = 0; i < l1; i++) {
                        a1->svals.push_back("");
                    }
                    break;
                default:
                    rerror("Id type error in DIM array: %s, [%t]", ik.id, id1->vtype);
                }
                vars[ik] = a1;
            }
            break;
        case S::ASSIGN: //赋值
            as1 = (Assign *) s;
            eval(as1->val);
            id1 = as1->id;
            ik.id = id1->id;
            if (id1->type == E::ID) {
                ik.type = V::ID;
                vi = vars.find(ik);
                if (vi != vars.end()) { //变量已定义
                    v1 = vi->second;
                    switch (v1->vtype) {
                    case E::VINT:
                        v1->ival = top.rval;
                        break;
                    case E::VREAL:
                        v1->rval = top.rval;
                        break;
                    case E::VSTRING:
                        v1->sval = top.sval;
                    }
                } else {
                    switch (id1->vtype) {
                    case E::VINT:
                        vars[ik] = gc.link(new V((long) top.rval));
                        break;
                    case E::VREAL:
                        vars[ik] = gc.link(new V(top.rval));
                        break;
                    case E::VSTRING:
                        vars[ik] = gc.link(new V(top.sval));
                        break;
                    }
                }
            } else { //数组
                ik.type = V::ARRAY;
                vi = vars.find(ik);
                ac1 = (Access *) id1;
                i1 = ac1->index.size();
                r1 = top.rval;
                s1 = top.sval;
                if (vi == vars.end()) { //数组不存在
                    l1 = 1;
                    a1 = gc.link(new Array);
                    for (i = 0; i < i1; i++) {
                        a1->bounds.push_back(11);
                        l1 *= 11;
                    }
                    switch (a1->vtype = id1->vtype) {
                    case E::VINT:
                        for (i = 0; i < l1; i++) {
                            a1->ivals.push_back(0L);
                        }
                        break;
                    case E::VREAL:
                        for (i = 0; i < l1; i++) {
                            a1->rvals.push_back(0.0);
                        }
                        break;
                    case E::VSTRING:
                        for (i = 0; i < l1; i++) {
                            a1->svals.push_back("");
                        }
                        break;
                    }
                    a1 = (Array *) (vars[ik] = a1);
                } else
                    a1 = (Array *) vi->second;
                eval(ac1->index[0]);
                l1 = top.rval;
                if (l1 >= a1->bounds[0]) {
                    rerror("Bad index in array assignment: %s [%i]", ik.id, l1);
                }
                for (i = 1; i < i1; i++) {
                    eval(ac1->index[i]);
                    i2 = top.rval;
                    if (i2 >= a1->bounds[i]) {
                        rerror("Bad index in array assignment: %s [%i]", ik.id, i2);
                    }
                    l1 = l1 * a1->bounds[i] + i2;
                }
                switch (id1->vtype) {
                case E::VINT:
                    a1->ivals[l1] = r1;
                    break;
                case E::VREAL:
                    a1->rvals[l1] = r1;
                    break;
                case E::VSTRING:
                    a1->svals[l1] = s1;
                }
            }
            break;
        case S::FOR: //只有第一次需要
            f1 = (For *) s;
            ik.id = f1->v;
            if (loops.empty()) {
                loops.push_back(s);
            } else {
                si = loops.end();
                do {
                    si--;
                    //自变量相同的for
                    if ((*si)->type == S::FOR && !((For *) *si)->v.compare(ik.id)) {
                        loops.erase(si, loops.end()); //清空内层循环及本循环
                        break;
                    }
                } while (si != loops.begin());
                loops.push_back(s);
            }
            eval(f1->dest);
            r1 = top.rval;
            ik.type = V::ID;
            v1 = vars[ik]; //自变量一定存在
            if (v1->vtype == E::VINT)
                r2 = v1->ival;
            else
                r2 = v1->rval;
            b1 = false;
            if (f1->step == 0 || (eval(f1->step), top.rval >= 0)) { //步长>=0
                if (r2 > r1) {
                    b1 = true;
                }
            } else if (r2 < r1) {
                b1 = true;
            }
            if (b1) { //查找对应的next
                loops.pop_back();
                s = s->next;
                i1 = 0;
                while (s) {
                    if (s->type == S::NEXT) {
                        if ((((Next *) s)->v.empty() && !i1
                                || !((Next *) s)->v.compare(ik.id))) {
                            break;
                        }
                        i1--;
                    }
                    if (s->type == S::FOR)
                        i1++;
                    s = s->next;
                }
                if (!s)
                    continue;
            }
            break;
        case S::NEXT:
            n1 = (Next *) s;
            if (n1->v.empty()) { //查找最近的for
                while (loops.size()) {
                    s = loops.back();
                    if (s->type == S::FOR) {
                        break;
                    }
                    loops.pop_back();
                }
                if (loops.empty()) {
                    rerror("Next without for");
                }
            } else { //查找自变量相同的for
                while (loops.size()) {
                    s = loops.back();
                    if (s->type == S::FOR && !((Next *) s)->v.compare(n1->v)) {
                        break;
                    }
                    loops.pop_back();
                }
                if (loops.empty()) {
                    rerror("Next without for: NEXT %s", n1->v);
                }
            }
            f1 = (For *) s;
            ik.id = f1->v;
            ik.type = V::ID;
            v1 = vars[ik];
            if (!f1->step) { //步长默认为1
                r2 = 1;
            } else {
                eval(f1->step);
                r2 = top.rval;
            }
            if (v1->vtype == E::VINT) {
                r1 = v1->ival += r2;
            } else {
                r1 = v1->rval += r2;
            }
            eval(f1->dest);
            if (r2 >= 0) {
                if (r1 > top.rval) {
                    loops.pop_back();
                    s = n1->next;
                }
            } else if (r1 < top.rval) {
                loops.pop_back();
                s = n1->next;
            }
            continue;
        case S::PRINT:
            p1 = (Print *) s;
            ss.precision(9);
            ss << uppercase;
            for (i = 0, i1 = p1->ps.size(); i < i1; i++) {
                if (e1 = p1->ps[i]) {
                    ss.str("");
                    if (e1->type == E::FUNC) {
                        f2 = (Func *) e1;
                        switch (f2->f) {
                        case F::TAB:
                            eval(f2->x);
                            if ((i2 = top.rval) < 0)
                                rerror("Illegal argument in PRINT: TAB [%i]", i2);
                            if (device->getX() >= i2)
                                device->nextLine();
                            device->locate(device->getY(), i2 - 1);
                            break;
                        case F::SPC:
                            eval(f2->x);
                            if ((i2 = top.rval) < 0)
                                rerror("Illegal argument in PRINT: SPC [%i]", i2);
                            for (l1 = 0; l1 < i2; l1++)
                                ss << ' ';
                            device->appendText(ss.str());
                            break;
                        default:
                            eval(e1);
                            if (top.vtype == E::VSTRING)
                                device->appendText(removeNull(remove0x1F(top.sval)));
                            else {
                                ss << top.rval;
                                device->appendText(ss.str());
                            }
                        }
                    } else {
                        eval(e1);
                            if (top.vtype == E::VSTRING) {
                                device->appendText(removeNull(remove0x1F(top.sval)));
                            } else {
                                ss << top.rval;
                                device->appendText(ss.str());
                            }
                    }
                }
                if (p1->del[i] == Print::CR && device->getX() > 0) {
                    //显示的文字刚好填满一行的时候就不用换行了
                    device->nextLine();
                }
            }
            device->updateLCD();
            break;
        case S::IF:
            if1 = (If *) s;
            eval(if1->con);
            if (notZero(top.rval)) {
                s = if1->stm;
            } else if (if1->elsestm) {
                s = if1->elsestm;
            } else
                break;
            continue;
        case S::GOTO:
            s = ((Goto *) s)->gotos;
            continue;
        case S::GOSUB:
            subs.push_back(s);
            s = ((Gosub *) s)->gotos;
            continue;
        case S::RETURN:
            if (subs.empty()) {
                rerror("Stack underflow: RETURN");
            }
            s = subs.back();
            subs.pop_back();
            break;
        case S::POP:
            if (subs.empty()) {
                rerror("Stack underflow: POP");
            }
            subs.pop_back();
            break;
        case S::ON:
            o1 = (On *) s;
            eval(o1->con);
            i1 = top.rval;
            if (i1 > 0 && i1 <= o1->stms.size()) {
                if (o1->isSub)
                    subs.push_back(o1);
                s = o1->stms[i1 - 1];
                continue;
            }
            break;
        case S::WHILE:
            if (loops.empty()) {
                loops.push_back(s);
            } else {
                si = loops.end();
                do {
                    si--;
                    //同一个while
                    if (*si == s) {
                        loops.erase(si, loops.end()); //清空内层循环及本循环
                        break;
                    }
                } while (si != loops.begin());
                loops.push_back(s);
            }
            eval(((While *) s)->con);
            if (isZero(top.rval)) { //查找对应的wend
                loops.pop_back();
                s = s->next;
                i1 = 0;
                while (s) {
                    if (s->type == S::WEND) {
                        if (!i1)
                            break;
                        i1--;
                    }
                    if (s->type == S::WHILE)
                        i1++;
                    s = s->next;
                }
                if (!s)
                    continue;
            }
            break;
        case S::WEND:
            t1 = s;
            while (loops.size()) {
                s = loops.back();
                if (s->type == S::WHILE)
                    break;
                loops.pop_back();
            }
            if (loops.empty()) {
                rerror("Wend without while");
            }
            eval(((While *) s)->con);
            if (isZero(top.rval)) {
                loops.pop_back();
                s = t1->next;
                continue;
            }
            break;
        case S::DEFFN:
            df = (DefFn *) s;
            if (funs.find(df->f) != funs.end()) { //函数已存在
                //rerror("Redefine function: %s", df->f);
                gc.remove(funs[df->f]);
            }
            funs[df->f] = gc.link(new Fn(df->vtype, df->x, df->xvtype, df->fn));
            break;
        case S::LOCATE:
            lc = (Locate *) s;
            if (lc->row) {
                eval(lc->row);
                i1 = top.rval - 1;
            } else {
                i1 = device->getY();
            }
            if (i1 < 0 || i1 > 4) {
                rerror("Illegal argument in LOCATE: row=%i", i1 + 1);
            }
            eval(lc->col);
            i2 = top.rval - 1;
            if (i2 < 0 || i2 > 19) {
                rerror("Illegal argument in LOCATE: col=%i", i2 + 1);
            }
            if (!lc->row && device->getX() > i2) {
                device->nextLine();
            }
            device->locate(i1, i2);
            break;
        case S::POKE: //不检查addr
            eval(((Poke *) s)->addr);
            i1 = top.rval;
            eval(((Poke *) s)->val);
            i2 = top.rval;
            if (i2 < 0 || i2 > 255) {
                rerror("Illegal value in POKE: %i", i2);
            }
            device->poke(i1, i2);
            break;
        case S::CALL:
            eval(((Call *) s)->addr);
            device->call(top.rval);
            break;
        case S::SWAP:
            sw = (Swap *) s;
            v1 = getValue(sw->id1);
            if (v1->type == V::ARRAY) {
                ac1 = (Access *) sw->id1;
                i1 = ac1->index.size();
                a1 = (Array *) v1;
                eval(ac1->index[0]);
                l1 = top.rval;
                if (l1 >= a1->bounds[0]) {
                    rerror("Bad index in SWAP: %s 0[%i]", ac1->id, l1);
                }
                for (i = 1; i < i1; i++) {
                    eval(ac1->index[i]);
                    i2 = top.rval;
                    if (i2 >= a1->bounds[i]) {
                        rerror("Bad index in SWAP: %s %i[%i]", ac1->id, i, i2);
                    }
                    l1 = l1 * a1->bounds[i] + i2;
                }
            }
            v2 = getValue(sw->id2);
            if (v2->type == V::ARRAY) {
                ac1 = (Access *) sw->id2;
                i1 = ac1->index.size();
                a2 = (Array *) v2;
                eval(ac1->index[0]);
                l2 = top.rval;
                if (l2 >= a2->bounds[0]) {
                    rerror("Bad index in SWAP: %s 0[%i]", ac1->id, l1);
                }
                for (i = 1; i < i1; i++) {
                    eval(ac1->index[i]);
                    i2 = top.rval;
                    if (i2 >= a2->bounds[i]) {
                        rerror("Bad index in SWAP: %s %i[%i]", ac1->id, i, i2);
                    }
                    l2 = l2 * a2->bounds[i] + i2;
                }
            }
            switch (v1->vtype) {
            case E::VINT:
                if (v1->type == V::ID) {
                    l3 = v1->ival;
                    if (v2->type == V::ID) {
                        v1->ival = v2->ival;
                    } else {
                        v1->ival = a2->ivals[l2];
                    }
                } else {
                    l3 = a1->ivals[l1];
                    if (v2->type == V::ID) {
                        a1->ivals[l1] = v2->ival;
                    } else {
                        a1->ivals[l1] = a2->ivals[l2];
                    }
                }
                if (v2->type == V::ID) {
                    v2->ival = l3;
                } else {
                    a2->ivals[l2] = l3;
                }
                break;
            case E::VREAL:
                if (v1->type == V::ID) {
                    r1 = v1->rval;
                    if (v2->type == V::ID) {
                        v1->rval = v2->rval;
                    } else {
                        v1->rval = a2->rvals[l2];
                    }
                } else {
                    r1 = a1->rvals[l1];
                    if (v2->type == V::ID) {
                        a1->rvals[l1] = v2->rval;
                    } else {
                        a1->rvals[l1] = a2->rvals[l2];
                    }
                }
                if (v2->type == V::ID) {
                    v2->rval = r1;
                } else {
                    a2->rvals[l2] = r1;
                }
                break;
            case E::VSTRING:
                if (v1->type == V::ID) {
                    if (v2->type == V::ID) {
                        v1->sval.swap(v2->sval);
                    } else {
                        v1->sval.swap(a2->svals[l2]);
                    }
                } else {
                    if (v2->type == V::ID) {
                        a1->svals[l1].swap(v2->sval);
                    } else {
                        a1->svals[l1].swap(a2->svals[l2]);
                    }
                }
                break;
            }
            break;
        case S::READ:
            rd = (Read *) s;
            for (i = 0, i1 = rd->ids.size(); i < i1; i++) {
                if (data.end()) {
                    rerror("Out of data: %s", rd->ids[i]->id);
                }
                v1 = getValue(rd->ids[i]);
                if (v1->type == V::ID) {
                    switch (v1->vtype) {
                    case E::VINT:
                        v1->ival = atol(data.get().c_str());
                        break;
                    case E::VREAL:
                        v1->rval = strtod(data.get().c_str(), 0);
                        break;
                    case E::VSTRING:
                        v1->sval = data.get();
                    }
                } else {
                    a1 = (Array *) v1;
                    ac1 = (Access *) rd->ids[i];
                    eval(ac1->index[0]);
                    l3 = ac1->index.size();
                    l1 = top.rval;
                    if (l1 >= a1->bounds[0]) {
                        rerror("Bad index in READ: %s 0[%i]", ac1->id, l1);
                    }
                    for (l2 = 1; l2 < l3; l2++) {
                        eval(ac1->index[l2]);
                        i2 = top.rval;
                        if (i2 >= a1->bounds[l2]) {
                            rerror("Bad index in READ: %s %i[%i]", ac1->id, (int) l2, i2);
                        }
                        l1 = l1 * a1->bounds[l2] + i2;
                    }
                    switch (a1->vtype) {
                    case E::VINT:
                        a1->ivals[l1] = atol(data.get().c_str());
                        break;
                    case E::VREAL:
                        a1->rvals[l1] = strtod(data.get().c_str(), 0);
                        break;
                    case E::VSTRING:
                        a1->svals[l1] = data.get();
                        break;
                    }
                }
            }
            break;
        case S::RESTORE:
            if (((Restore *) s)->rlabel == Restore::NOLABEL) {
                data.restore();
            } else {
                data.restore(((Restore *) s)->rlabel);
            }
            break;
        case S::LSET:
            ls = (Lset *) s;
            v1 = getValue(ls->id);
            if (v1->type == V::ID) {
                eval(ls->s);
                i2 = top.sval.length();
                i1 = v1->sval.length();
                if (i1 > i2) {
                    v1->sval.replace(0, i2, top.sval);
                } else
                    v1->sval = string(top.sval.begin(), top.sval.begin() + i1);
            } else {
                a1 = (Array *) v1;
                ac1 = (Access *) ls->id;
                eval(ac1->index[0]);
                i1 = ac1->index.size();
                l1 = top.rval;
                if (l1 >= a1->bounds[0]) {
                    rerror("Bad index in LSET: %s 0[%i]", ls->id->id, l1);
                }
                for (int i = 1; i < i1; i++) {
                    eval(ac1->index[i]);
                    i2 = top.rval;
                    if (i2 >= a1->bounds[i]) {
                        rerror("Bad index in LSET: %s %i[%i]", ls->id->id, i, i2);
                    }
                    l1 = l1 * a1->bounds[i] + i2;
                }
                eval(ls->s);
                i2 = top.sval.length();
                i1 = a1->svals[l1].length();
                if (i1 > i2) {
                    a1->svals[l1].replace(0, i2, top.sval);
                } else {
                    a1->svals[l1] = string(top.sval.begin(), top.sval.begin() + i1);
                }
            }
            break;
        case S::RSET:
            rs = (Rset *) s;
            v1 = getValue(rs->id);
            if (v1->type == V::ID) {
                eval(rs->s);
                i2 = top.sval.length();
                i1 = v1->sval.length();
                if (i1 > i2) {
                    v1->sval.replace(i1 - i2, i2, top.sval);
                } else {
                    v1->sval = string(top.sval.end() - i1, top.sval.end());
                }
            } else {
                a1 = (Array *) v1;
                ac1 = (Access *) rs->id;
                eval(ac1->index[0]);
                i1 = ac1->index.size();
                l1 = top.rval;
                if (l1 >= a1->bounds[0]) {
                    rerror("Bad index in LSET: %s 0[%i]", rs->id->id, l1);
                }
                for (int i = 1; i < i1; i++) {
                    eval(ac1->index[i]);
                    i2 = top.rval;
                    if (i2 >= a1->bounds[i]) {
                        rerror("Bad index in LSET: %s %i[%i]", rs->id->id, i, i2);
                    }
                    l1 = l1 * a1->bounds[i] + i2;
                }
                eval(rs->s);
                i2 = top.sval.length();
                i1 = a1->svals[l1].length();
                if (i1 > i2) {
                    a1->svals[l1].replace(i1 - i2, i2, top.sval);
                } else {
                    a1->svals[l1] = string(top.sval.end() - i1, top.sval.end());
                }
            }
            break;
        case S::CLOSE:
            if (!fileman.isOpen(((Close *) s)->fnum)) {
                rerror("File not open: CLOSE #%i", ((Close *) s)->fnum + 1);
            }
            fileman.close(((Close *) s)->fnum);
            break;
        case S::OPEN:
            op = (Open *) s;
            eval(op->fname);
            if (fileman.isOpen(op->fnum)) {
                rerror("Reopen file: %s", top.sval);
            }
            i1 = op->fnum;
            //append extension
            s1 = top.sval;
            if (s1.length() < 4) {
                top.sval.append(".DAT");
            } else {
                i2 = s1.length() - 1;
                if (s1[i2 - 3] != '.' || (s1[i2 - 2] & 0xdf) != 'D' ||
                    (s1[i2 - 1] & 0xdf) != 'A' || (s1[i2] & 0xdf) != 'T') {
                    top.sval.append(".DAT");
                }
            }
            switch (op->mode) {
            case Open::MINPUT:
                fileman.open(i1, top.sval, FM::INPUT);
                break;
            case Open::MOUTPUT:
                fileman.open(i1, top.sval, FM::OUTPUT);
                break;
            case Open::MAPPEND:
                fileman.open(i1, top.sval, FM::APPEND);
                break;
            case Open::MRANDOM:
                fileman.open(i1, top.sval, FM::RANDOM);
                records[i1].len = op->len;
                records[i1].total = Open::NOLEN;
                break;
            case Open::MBINARY:
                fileman.open(i1, top.sval, FM::INPUT);
                if (fileman.isOpen(i1)) { //文件存在
                    fileman.close(i1);
                    fileman.open(i1, top.sval, FM::RANDOM);
                }
                break;
            }
            if (!fileman.isOpen(i1) && op->mode != Open::MBINARY) {
                rerror("File open error: %s", top.sval);
            }
            break;
        case S::WRITE:
            wr = (Write *) s;
            i1 = wr->fnum;
            if (!fileman.isOpen(i1)) {
                rerror("File not open: WRTIE #%i", i1 + 1);
            }
            if (fileman.mode(i1) != FM::OUTPUT
                    && fileman.mode(i1) != FM::APPEND) {
                rerror("File mode error: WRITE #%i", i1 + 1);
            }
            for (i = 0, i2 = wr->es.size(); i < i2; i++) {
                eval(wr->es[i]);
                if (top.vtype == E::VSTRING) {
                    fileman.writeByte(i1, '"');
                    fileman.writeString(i1, remove0x1F(top.sval));
                    fileman.writeByte(i1, '"');
                } else {
                    fileman.writeReal(i1, top.rval);
                }
                fileman.writeByte(i1, i < i2 - 1 ? ',' : 0xff);
            }
            break;
        case S::FINPUT:
            fip = (FInput *) s;
            i1 = fip->fnum;
            if (!fileman.isOpen(i1)) {
                rerror("File not open: INPUT #%i", i1 + 1);
            }
            if (fileman.mode(i1) != FM::INPUT) {
                rerror("File mode error: INPUT #%i", i1 + 1);
            }
            for (i = 0, i2 = fip->ids.size(); i < i2; i++) {
                v1 = getValue(fip->ids[i]);
                if (fileman.eof(i1)) {
                    rerror("EOF reached: INPUT #%i, %s", i1 + 1, fip->ids[i]->id);
                }
                if (v1->type == V::ID) {
                    switch (v1->vtype) {
                    case E::VINT:
                        v1->ival = fileman.readReal(i1);
                        break;
                    case E::VREAL:
                        v1->rval = fileman.readReal(i1);
                        break;
                    case E::VSTRING:
                        v1->sval = fileman.readString(i1);
                    }
                } else {
                    ac1 = (Access *) fip->ids[i];
                    a1 = (Array *) v1;
                    eval(ac1->index[0]);
                    l1 = top.rval;
                    if (l1 >= a1->bounds[0]) {
                        rerror("Bad index in INPUT #%i: %s 0[%i]", i1 + 1, ac1->id, l1);
                    }
                    for (l2 = 1, l3 = ac1->index.size(); l2 < l3; l2++) {
                        eval(ac1->index[l2]);
                        i0 = top.rval;
                        if (i0 >= a1->bounds[l2]) {
                            rerror("Bad index in INPUT #%i: %s %i[%i]", i1 + 1, ac1->id,
                                l2, i0);
                        }
                        l1 = l1 * a1->bounds[l2] + i0;
                    }
                    switch (v1->vtype) {
                    case E::VINT:
                        a1->ivals[l1] = fileman.readReal(i1);
                        break;
                    case E::VREAL:
                        a1->rvals[l1] = fileman.readReal(i1);
                        break;
                    case E::VSTRING:
                        a1->svals[l1] = fileman.readString(i1);
                    }
                }
                fileman.skip(i1);
            }
            break;
        case S::INPUT:
            ip = (Input *) s;
            device->appendText(ip->prm); //显示prompt
            device->updateLCD();
            for (i = 0, i2 = ip->ids.size(); i < i2; i++) {
                v1 = getValue(ip->ids[i]);
                if (v1->type == V::ID) {
                    switch (v1->vtype) {
                    case E::VINT:
                        v1->ival = atol(device->input(ip->prm, Device::NUM).c_str());
                        break;
                    case E::VREAL:
                        v1->rval = strtod(device->input(ip->prm, Device::NUM).c_str(), 0);
                        break;
                    case E::VSTRING:
                        v1->sval = device->input(ip->prm, Device::STR);
                    }
                } else {
                    ac1 = (Access *) ip->ids[i];
                    a1 = (Array *) v1;
                    eval(ac1->index[0]);
                    l1 = top.rval;
                    if (l1 >= a1->bounds[0]) {
                        rerror("Bad index in INPUT: %s 0[%i]", i1, ac1->id, l1);
                    }
                    for (l2 = 1, l3 = ac1->index.size(); l2 < l3; l2++) {
                        eval(ac1->index[l2]);
                        i0 = top.rval;
                        if (i0 >= a1->bounds[l2]) {
                            rerror("Bad index in INPUT: %s %i[%i]", i1, ac1->id, l2, i0);
                        }
                        l1 = l1 * a1->bounds[l2] + i0;
                    }
                    switch (v1->vtype) {
                    case E::VINT:
                        a1->ivals[l1] = atol(device->input(ip->prm, Device::NUM).c_str());
                        break;
                    case E::VREAL:
                        a1->rvals[l1] = strtod(device->input(ip->prm, Device::NUM).c_str(), 0);
                        break;
                    case E::VSTRING:
                        a1->svals[l1] = device->input(ip->prm, Device::STR);
                    }
                }
            }
            //device->getkey(); //啊啊啊啊啊啊啊啊啊
            break;
        case S::FIELD:
            {
                fl = (Field *) s;
                i1 = fl->fnum;
                if (!fileman.isOpen(i1)) {
                    rerror("File not open: FIELD #%i", i1 + 1);
                }
                if (fileman.mode(i1) != FM::RANDOM) {
                    rerror("File mode error: FIELD #%i", i1 + 1);
                }
                Record &rec = records[i1];
                if (rec.len != Open::NOLEN && rec.len < fl->total) {
                    rerror("Record size overflow: FIELD #%i: %i, LEN=%i", i1 + 1,
                        fl->total, rec.len);
                }
                rec.ids.clear();
                rec.bys.clear();
                rec.total = fl->total;
                if (rec.len == Open::NOLEN) {
                    rec.len = rec.total;
                }

                ik.type = V::ID;
                for (i = 0, i2 = fl->ids.size(); i < i2; i++) {
                    rec.ids.push_back(ik.id = fl->ids[i]);
                    rec.bys.push_back(i0 = fl->bys[i]);
                    vi = vars.find(ik);
                    if (vi == vars.end()) {
                        vars[ik] = gc.link(new V(string(i0, '\0')));
                    } else {
                        vi->second->sval = string(i0, '\0');
                    }
                }
            }
            break;
        case S::PUT:
            {
                fp = (Put *) s;
                i1 = fp->fnum;
                if (!fileman.isOpen(i1)) {
                    rerror("File not open: PUT #%i", i1 + 1);
                }
                if (fileman.mode(i1) != FM::RANDOM) {
                    rerror("File mode error: PUT #%i", i1 + 1);
                }
                Record &rec = records[i1];
                if (rec.total == Open::NOLEN) {
                    rerror("Record not assigned: PUT #%i", i1 + 1);
                }
                eval(fp->rec);
                i0 = top.rval - 1;
                if (i0 < 0 || i0 * rec.total > fileman.size(i1)) {
                    rerror("Bad record number: PUT #%i, %i", i1 + 1, i0 + 1);
                }
                fileman.seek(i1, i0 * rec.len);
                ik.type = V::ID;
                for (i = 0, i2 = rec.ids.size(); i < i2; i++) {
                    ik.id = rec.ids[i];
                    fileman.writeString(i1, vars[ik]->sval);
                }
                if (rec.len != Open::NOLEN && rec.len > rec.total) {
                    for (i = 0, i2 = rec.len - rec.total; i < i2; i++) {
                        fileman.writeByte(i1, '\0');
                    }
                }
            }
            break;
        case S::GET:
            {
                fg = (Get *) s;
                i1 = fg->fnum;
                if (!fileman.isOpen(i1)) {
                    rerror("File not open: GET #%i", i1 + 1);
                }
                if (fileman.mode(i1) != FM::RANDOM) {
                    rerror("File mode error: GET #%i", i1 + 1);
                }
                Record &rec = records[i1];
                if (rec.total == Open::NOLEN) {
                    rerror("Record not assigned: GET #%i", i1 + 1);
                }
                eval(fg->rec);
                i0 = top.rval - 1;
                if (i0 < 0 || (i0 + 1) * rec.total > fileman.size(i1)) {
                    rerror("Bad record number: GET #%i, %i", i1 + 1, i0 + 1);
                }
                fileman.seek(i1, i0 * rec.len);
                ik.type = V::ID;
                for (i = 0, i2 = rec.ids.size(); i < i2; i++) {
                    ik.id = rec.ids[i];
                    vars[ik]->sval = fileman.readString(i1, rec.bys[i]);
                }
                if (rec.len != Open::NOLEN && rec.len > rec.total) {
                    for (i = 0, i2 = rec.len - rec.total; i < i2; i++) {
                        fileman.readByte(i1);
                    }
                }
            }
            break;
        case S::DRAW:
            pd = (Draw *) s;
            eval(pd->x);
            i1 = top.rval;
            if (i1 < 0) {
                rerror("Illegal x value in DRAW: %i", i1);
            }
            eval(pd->y);
            i2 = top.rval;
            if (i2 < 0) {
                rerror("Illegal y value in DRAW: %i", i2);
            }
            if (!pd->t) {
                i = 1;
            } else {
                eval(pd->t);
                i = ((int) top.rval) & 3;
            }
            device->point(i1, i2, i);
            break;
        case S::LINE:
            pl = (Line *) s;
            eval(pl->x1);
            i0 = top.rval;
            if (i0 < 0) {
                rerror("Illegal x1 value in LINE: %i", i0);
            }
            eval(pl->y1);
            i1 = top.rval;
            if (i1 < 0) {
                rerror("Illegal y1 value in LINE: %i", i1);
            }
            eval(pl->x2);
            i2 = top.rval;
            if (i2 < 0) {
                rerror("Illegal x2 value in LINE: %i", i2);
            }
            eval(pl->y2);
            i3 = top.rval;
            if (i3 < 0) {
                rerror("Illegal y1 value in LINE: %i", i3);
            }
            if (!pl->t) {
                i = 1;
            } else {
                eval(pl->t);
                i = ((int) top.rval) & 3;
            }
            device->line(i0, i1, i2, i3, i);
            break;
        case S::BOX:
            pb = (Box *) s;
            eval(pb->x1);
            i0 = top.rval;
            if (i0 < 0) {
                rerror("Illegal x1 value in BOX: %i", i0);
            }
            eval(pb->y1);
            i1 = top.rval;
            if (i1 < 0) {
                rerror("Illegal y1 value in BOX: %i", i1);
            }
            eval(pb->x2);
            i2 = top.rval;
            if (i2 < 0) {
                rerror("Illegal x2 value in BOX: %i", i2);
            }
            eval(pb->y2);
            i3 = top.rval;
            if (i3 < 0) {
                rerror("Illegal y1 value in BOX: %i", i3);
            }
            if (!pb->f) {
                b1 = false;
            } else {
                eval(pb->f);
                b1 = notZero(top.rval);
            }
            if (!pb->t) {
                i = 1;
            } else {
                eval(pb->t);
                i = ((int) top.rval) & 3;
            }
            device->rectangle(i0, i1, i2, i3, b1, i);
            break;
        case S::CIRCLE:
            pc = (Circle *) s;
            eval(pc->x);
            i1 = top.rval;
            if (i1 < 0) {
                rerror("Illegal x value in CIRCLE: %i", i1);
            }
            eval(pc->y);
            i2 = top.rval;
            if (i2 < 0) {
                rerror("Illegal y value in CIRCLE: %i", i2);
            }
            eval(pc->r);
            i3 = top.rval;
            if (i3 < 0) {
                rerror("Illegal radius value in CIRCLE: %i", i3);
            }
            if (!pc->f) {
                b1 = false;
            } else {
                eval(pc->f);
                b1 = notZero(top.rval);
            }
            if (!pc->t) {
                i = 1;
            } else {
                eval(pc->t);
                i = ((int) top.rval) & 3;
            }
            device->ellipse(i1, i2, i3, i3, b1, i);
            break;
        case S::ELLIPSE:
            pe = (Ellipse *) s;
            eval(pe->x);
            i1 = top.rval;
            if (i1 < 0) {
                rerror("Illegal x value in ELLIPSE: %i", i1);
            }
            eval(pe->y);
            i2 = top.rval;
            if (i2 < 0) {
                rerror("Illegal y value in ELLIPSE: %i", i2);
            }
            eval(pe->xr);
            i3 = top.rval;
            if (i3 < 0) {
                rerror("Illegal x-radius value in ELLIPSE: %i", i3);
            }
            eval(pe->yr);
            i4 = top.rval;
            if (i4 < 0) {
                rerror("Illegal y-radius value in ELLIPSE: %i", i4);
            }
            if (!pe->f) {
                b1 = false;
            } else {
                eval(pe->f);
                b1 = notZero(top.rval);
            }
            if (!pe->t) {
                i = 1;
            } else {
                eval(pe->t);
                i = ((int) top.rval) & 3;
            }
            device->ellipse(i1, i2, i3, i4, b1, i);
            break;
            //自定义
        case S::SLEEP:
            eval(((SSleep *) s)->ms);
            device->sleep(top.rval);
            break;
        case S::PAINT:
            {
                SPaint *sp = (SPaint *) s;
                eval(sp->addr);
                i = top.rval;
                eval(sp->x);
                i1 = top.rval;
                eval(sp->y);
                i2 = top.rval;
                eval(sp->w);
                i3 = top.rval;
                eval(sp->h);
                i4 = top.rval;
                if (sp->mode) {
                    eval(sp->mode);
                    i0 = top.rval;
                } else {
                    i0 = 0;
                }
                device->paint(i, i1, i2, i3, i4, i0);
            }
            break;
        case S::LOAD:
            {
                SLoad *sl = (SLoad *) s;
                eval(sl->addr);
                device->poke(top.rval, sl->values.c_str(), sl->values.size());
            }
            break;
        case S::FPUTC:
            {
                SFputc *sfp = (SFputc *) s;
                i1 = sfp->fnum;
                if (!fileman.isOpen(i1)) {
                    rerror("File not open: FPUTC #%i", i1 + 1);
                }
                if (fileman.mode(i1) != FM::RANDOM) {
                    rerror("File mode error: FPUTC #%i", i1 + 1);
                }
                eval(sfp->ch);
                if (top.sval.empty()) {
                    rerror("Empty string: FPUTC #%i", i1 + 1);
                }
                fileman.writeByte(i1, top.sval[0]);
            }
            break;
        case S::FSEEK:
            {
                SFseek *sfs = (SFseek *) s;
                i1 = sfs->fnum;
                if (!fileman.isOpen(i1)) {
                    rerror("File not open: FSEEK #%i", i1 + 1);
                }
                if (fileman.mode(i1) != FM::RANDOM) {
                    rerror("File mode error: FSEEK #%i", i1 + 1);
                }
                eval(sfs->pt);
                fileman.seek(i1, top.rval);
            }
            break;
        case S::FREAD:
            {
                SFread *sfr = (SFread *) s;
                i1 = sfr->fnum;
                if (!fileman.isOpen(i1)) {
                    rerror("File not open: FREAD #%i", i1 + 1);
                }
                if (fileman.mode(i1) != FM::RANDOM) {
                    rerror("File mode error: FREAD #%i", i1 + 1);
                }
                eval(sfr->addr);
                i0 = top.rval;
                eval(sfr->size);
                s1 = fileman.readString(i1, top.rval);
                device->poke(i0, s1.c_str(), s1.length());
            }
            break;
        case S::FWRITE:
            {
                SFwrite *sfw = (SFwrite *) s;
                i1 = sfw->fnum;
                if (!fileman.isOpen(i1)) {
                    rerror("File not open: FWRITE #%i", i1 + 1);
                }
                if (fileman.mode(i1) != FM::RANDOM) {
                    rerror("File mode error: FWRITE #%i", i1 + 1);
                }
                eval(sfw->addr);
                i0 = top.rval;
                eval(sfw->size);
                s1 = device->peek(i0, top.rval);
                fileman.writeString(i1, s1);
            }
            break;
        }
        s = s->next;
        delay();
    }
}

V *GVB::getValue(Id *id) { //占用top，vtype可能是vint
    IdKey ik;
    variter vi;
    Array *a;
    Access *ac;
    int i, j;
    long l;

    ik.id = id->id;
    if (id->type == E::ID) {
        ik.type = V::ID;
        vi = vars.find(ik);
        if (vi == vars.end()) { //变量不存在
            switch (id->vtype) {
            case E::VINT:
                return vars[ik] = gc.link(new V(0L));
            case E::VREAL:
                return vars[ik] = gc.link(new V(0.0));
            case E::VSTRING:
                return vars[ik] = gc.link(new V(""));
            }
        }
        return vi->second;
    } else {
        ac = (Access *) id;
        j =  ac->index.size();
        ik.type = V::ARRAY;
        vi = vars.find(ik);
        if (vi == vars.end()) { //数组不存在
            l = 1;
            a = gc.link(new Array);
            for (i = 0; i < j; i++) {
                a->bounds.push_back(11);
                l *= 11;
            }
            switch (a->vtype = ac->vtype) {
            case E::VINT:
                for (i = 0; i < l; i++) {
                    a->ivals.push_back(0L);
                }
                break;
            case E::VREAL:
                for (i = 0; i < l; i++) {
                    a->rvals.push_back(0.0);
                }
                break;
            case E::VSTRING:
                for (i = 0; i < l; i++) {
                    a->svals.push_back("");
                }
                break;
            }
            return vars[ik] = a;
        }
        return vi->second;
    }
}

void GVB::eval(E *e) {
    eval_(e);
    pop();
}

void GVB::eval_(E *e) {
    if (terminated()) {
        throw 1;
    }
    Id *id1;
    Access *ac1;
    Func *f1;
    Binary *b1;
    V *v1;
    V vt;
    Array *a1;
    IdKey ik;
    CallFn *cf;
    Fn *f2;
    variter vi;
    int i, i1, i2;
    long l1;
    char c[2];
    string s1;
    funiter fi;

    switch (e->type) {
    case E::ID:
        id1 = (Id *) e;
        ik.id = id1->id;
        ik.type = V::ID;
        vi = vars.find(ik);
        if (vi == vars.end()) { //变量不存在
            switch (getIdType(ik.id)) {
            case E::VINT:
                vars[ik] = gc.link(new V(0L));
                break;
            case E::VREAL:
                vars[ik] = gc.link(new V(0.0));
                break;
            case E::VSTRING:
                vars[ik] = gc.link(new V(""));
                break;
            }
            vi = vars.find(ik);
        }
        v1 = vi->second;
        if (v1->vtype == E::VINT) {
            push(v1->ival);
        } else
            push(v1);
        break;
    case E::ACCESS:
        ac1 = (Access *) e;
        i1 =  ac1->index.size();
        ik.id = ac1->id;
        ik.type = V::ARRAY;
        vi = vars.find(ik);
        if (vi == vars.end()) { //数组不存在
            l1 = 1;
            a1 = gc.link(new Array);
            for (i = 0; i < i1; i++) {
                a1->bounds.push_back(11);
                l1 *= 11;
            }
            switch (a1->vtype = getIdType(ik.id)) {
            case E::VINT:
                for (i = 0; i < l1; i++) {
                    a1->ivals.push_back(0L);
                }
                break;
            case E::VREAL:
                for (i = 0; i < l1; i++) {
                    a1->rvals.push_back(0.0);
                }
                break;
            case E::VSTRING:
                for (i = 0; i < l1; i++) {
                    a1->svals.push_back("");
                }
                break;
            }
            vars[ik] = a1;
            vi = vars.find(ik);
        }
        a1 = (Array *) vi->second;
        eval(ac1->index[0]);
        l1 = top.rval;
        if (l1 >= a1->bounds[0]) {
            rerror("Bad index in array: %s 0[%i]", ik.id, l1);
        }
        for (i = 1; i < i1; i++) {
            eval(ac1->index[i]);
            i2 = top.rval;
            if (i2 >= a1->bounds[i]) {
                rerror("Bad index in array: %s %i[%i]", ik.id, i, i2);
            }
            l1 = l1 * a1->bounds[i] + i2;
        }
        switch (a1->vtype) {
        case E::VINT:
            push(a1->ivals[l1]);
            break;
        case E::VREAL:
            push(a1->rvals[l1]);
            break;
        case E::VSTRING:
            push(a1->svals[l1]);
        }
        break;
    case E::REAL:
        push(((Real *) e)->rval);
        break;
    case E::STRING:
        push(((Str *) e)->sval);
        break;
    case E::INKEY:
        *c = device->getkey(); //不可能是0
        c[1] = 0;
        push(c);
        break;
    case E::FUNC:
        f1 = (Func *) e;
        eval(f1->x);
        switch (f1->f) {
        case F::NEG:
            push(-top.rval);
            break;
        case F::NOT:
            push(isZero(top.rval));
            break;
        case F::ABS:
            push(fabs(top.rval));
            break;
        case F::ASC:
            if (top.sval.empty()) {
                rerror("Illegal argument in ASC: empty string");
            }
            push(top.sval[0] & 0xff);
            break;
        case F::ATN:
            errno = 0;
            push(atan(top.rval));
            if (errno) {
                rerror("Illegal argument: ATN(%f)", top.rval);
            }
            break;
        case F::CHR:
            i1 = top.rval;
            if (i1 < 0 || i1 > 255) {
                rerror("Illegal argument: CHR(%i)", i1);
            }
            c[0] = i1;
            push(string(c, c + 1)); //不能直接push(c)！c[0]可能是0！
            break;
        case F::COS:
            push(cos(top.rval));
            break;
        case F::CVI: //2byte -> int
            if (top.sval.length() < 2) {
                rerror("String length not enough in CVI: len(%s) = %i", top.sval,
                    (int) top.sval.length());
            }
            push(*(short *) top.sval.c_str());
            break;
        case F::MKI: //int -> 2byte
            *(short *) c = top.rval;
            push(string(c, c + 2));
            break;
        case F::CVS: //8byte -> double
            if (top.sval.length() < 8) {
                rerror("String length not enough in CVS: len(%s) = %i", top.sval,
                    (int) top.sval.length());
            }
            push(*(double *) top.sval.c_str());
            break;
        case F::MKS: //double -> 8byte
            push(string((char *) &top.rval, (char *) &top.rval + 8));
            break;
        case F::EXP:
            push(exp(top.rval));
            break;
        case F::INT:
            push(floor(top.rval));
            break;
        case F::LEN:
            push(top.sval.length());
            break;
        case F::LEFT:
            s1 = top.sval;
            eval(f1->x2);
            i1 = top.rval;
            if (i1 < 0) {
                rerror("Illegal count in LEFT: %i", i1);
            }
            if (s1.length() < i1)
                i1 = s1.length();
            push(string(s1.begin(), s1.begin() + i1));
            break;
        case F::RIGHT:
            s1 = top.sval;
            eval(f1->x2);
            i1 = top.rval;
            if (i1 < 0) {
                rerror("Illegal count in RIGHT: %i", i1);
            }
            if (s1.length() < i1)
                i1 = s1.length();
            push(string(s1.end() - i1, s1.end()));
            break;
        case F::LOG:
            errno = 0;
            push(log(top.rval));
            if (errno) {
                rerror("Illegal argument: LOG(%f)", top.rval);
            }
            break;
        case F::MID:
            s1 = top.sval;
            eval(f1->x2);
            i1 = top.rval;
            if (i1 < 1 || i1 > s1.length()) {
                rerror("Illegal offset in MID: %i, len(%s) = %i", i1, s1, s1.length());
            }
            if (!f1->x3) {
                i2 = 1;
            } else {
                eval(f1->x3);
                i2 = top.rval;
            }
            if (i2 < 1) {
                rerror("Illegal count in MID: %i", i2);
            }
            i1--;
            if (i1 + i2 > s1.length()) {
                i2 = s1.length() - i1;
            }
            push(s1.substr(i1, i2));
            break;
        case F::POS:
            push(device->getX() + 1);
            break;
        case F::RND:
            if (isZero(top.rval)) {
                push((double) seed / (RAND + 1));
            } else if (top.rval > 0) {
                push((double) random() / (RAND + 1));
            } else {
                push((double) sequence() / (RAND + 1));
            }
            break;
        case F::SGN:
            if (isZero(top.rval)) {
                push(0.0);
            } else if (top.rval > 0) {
                push(1.0);
            } else
                push(-1.0);
            break;
        case F::SIN:
            push(sin(top.rval));
            break;
        case F::SQR:
            errno = 0;
            push(sqrt(top.rval));
            if (errno) {
                rerror("Illegal argument: SQR(%f)", top.rval);
            }
            break;
        case F::STR:
            {
                stringstream ss;
                ss.precision(9);
                ss << top.rval;
                push(ss.str());
            }
            break;
        case F::TAN:
            errno = 0;
            push(tan(top.rval));
            if (errno) {
                rerror("Illegal argument: TAN(%f)", top.rval);
            }
            break;
        case F::VAL:
            push(strtod(top.sval.c_str(), 0));
            break;
        case F::PEEK: //不检查参数合法性
            push((byte) device->peek(top.rval));
            break;
        case F::FEOF:
            i1 = top.rval - 1;
            if (i1 < 0 || i1 > 2) {
                rerror("Illegal file number: EOF(%i)", i1 + 1);
            }
            if (!fileman.isOpen(i1)) {
                rerror("File not open: EOF(%i)", i1 + 1);
            }
            if (fileman.mode(i1) != FM::INPUT) {
                rerror("File mode error: EOF(%i), mode: %i", i1 + 1, fileman.mode(i1));
            }
            push(fileman.eof(i1));
            break;
        case F::LOF:
            i1 = top.rval - 1;
            if (i1 < 0 || i1 > 2) {
                rerror("Illegal file number: LOF(%i)", i1 + 1);
            }
            if (!fileman.isOpen(i1)) {
                rerror("File not open: LOF(%i)", i1 + 1);
            }
            if (fileman.mode(i1) != FM::RANDOM) {
                rerror("File mode error: LOF(%i), mode: %i", i1 + 1, fileman.mode(i1));
            }
            push(fileman.size(i1));
            break;
            //自定义函数
        case F::CHECKKEY:
            push(device->checkKey(top.rval));
            break;
        case F::POINT:
            i1 = top.rval;
            eval(f1->x2);
            push(device->getPoint(i1, top.rval));
            break;
        case F::FOPEN: //文件是否打开
            push(fileman.isOpen(top.rval - 1));
            break;
        case F::FGETC:
            i1 = top.rval - 1;
            if (!fileman.isOpen(i1)) {
                rerror("File not open: FGETC(%i)", i1 + 1);
            }
            if (fileman.mode(i1) != FM::RANDOM) {
                rerror("File mode error: FGETC(%i), mode: %i", i1 + 1, fileman.mode(i1));
            }
            push(fileman.readByte(i1) & 0xff);
            break;
        case F::FTELL:
            i1 = top.rval - 1;
            if (!fileman.isOpen(i1)) {
                rerror("File not open: FTELL(%i)", i1 + 1);
            }
            if (fileman.mode(i1) != FM::RANDOM) {
                rerror("File mode error: FTELL(%i), mode: %i", i1 + 1, fileman.mode(i1));
            }
            push(fileman.tell(i1));
            break;
        default:
            rerror("Unknown function: %i", f1->f);
        }
        break;
    case E::BINARY:
        b1 = (Binary *) e;
        eval(b1->l);
        vt.vtype = top.vtype;
        vt.sval = top.sval;
        vt.rval = top.rval;
        eval(b1->r);
        switch (b1->op) {
        case '+':
            if (vt.vtype == E::VREAL)
                push(vt.rval + top.rval);
            else
                push(vt.sval + top.sval);
            break;
        case '-':
            push(vt.rval - top.rval);
            break;
        case '*':
            push(vt.rval * top.rval);
            break;
        case '/':
            if (top.rval == 0) {
                rerror("Division by zero");
            }
            push(vt.rval / top.rval);
            break;
        case '^':
            push(pow(vt.rval, top.rval));
            break;
        case '=':
            if (vt.vtype == E::VREAL)
                push(isZero(vt.rval - top.rval));
            else
                push(vt.sval.compare(top.sval) == 0);
            break;
        case L::NEQ:
            if (vt.vtype == E::VREAL)
                push(notZero(vt.rval - top.rval));
            else
                push(vt.sval.compare(top.sval) != 0);
            break;
        case L::GE:
            if (vt.vtype == E::VREAL)
                push(vt.rval > top.rval || isZero(vt.rval - top.rval));
            else
                push(vt.sval.compare(top.sval) >= 0);
            break;
        case L::LE:
            if (vt.vtype == E::VREAL)
                push(vt.rval < top.rval || isZero(vt.rval - top.rval));
            else
                push(vt.sval.compare(top.sval) <= 0);
            break;
        case '>':
            if (vt.vtype == E::VREAL)
                push(vt.rval > top.rval);
            else
                push(vt.sval.compare(top.sval) > 0);
            break;
        case '<':
            if (vt.vtype == E::VREAL)
                push(vt.rval < top.rval);
            else
                push(vt.sval.compare(top.sval) < 0);
            break;
        case L::AND:
            push(notZero(vt.rval) && notZero(top.rval));
            break;
        case L::OR:
            push(notZero(vt.rval) || notZero(top.rval));
            break;
        }
        break;
    case E::CLFN:
        cf = (CallFn *) e;
        fi = funs.find(cf->fn);
        if (fi == funs.end()) {
            rerror("Function undefined: %s", cf->fn);
        }
        f2 = fi->second;
        if ((f2->xvtype & E::RVAL_MASK) != cf->x->vtype) {
            rerror("Incompatible type in FN %s(%s: %t): [%t]", cf->fn, f2->x, f2->xvtype,
                cf->x->vtype);
        }
        eval(cf->x);
        ik.id = f2->x;
        ik.type = V::ID;
        vi = vars.find(ik);
        if (vi != vars.end()) { //变量已存在
            vt = *(v1 = vi->second);
            i = 1;
            switch (vt.vtype) {
            case E::VINT:
                v1->ival = top.rval;
                break;
            case E::VREAL:
                v1->rval = top.rval;
                break;
            case E::VSTRING:
                v1->sval = top.sval;
            }
        } else {
            i = 0;
            switch (f2->xvtype) {
            case E::VINT:
                vars[ik] = v1 = gc.link(new V((long) top.rval));
                break;
            case E::VREAL:
                vars[ik] = v1 = gc.link(new V(top.rval));
                break;
            case E::VSTRING:
                vars[ik] = v1 = gc.link(new V(top.sval));
            }
        }
        eval_(f2->func);
        if (i) {
            switch (vt.vtype) {
            case E::VINT:
                v1->ival = vt.ival;
                break;
            case E::VREAL:
                v1->rval = vt.rval;
                break;
            case E::VSTRING:
                v1->sval = vt.sval;
            }
        } else {
            vars.erase(ik);
            gc.remove(v1);
        }
        if (f2->vtype == E::VINT) {
            pop();
            push((long) top.rval);
        }
        break;
    default:
        rerror("Unknown expression type: [%i]", e->type);
    }
}

void GVB::randomize(unsigned _seed) {
    seed = _seed;
}

unsigned GVB::random() {
    return seed = (seed * 2053 + 13849) & RAND;
}

unsigned GVB::sequence() {
    return seed = (seed + 13849) & RAND;
}

int GVB::getCurrentLabel() const {
    return label;
}

int GVB::getCurrentLine() const {
    return line;
}