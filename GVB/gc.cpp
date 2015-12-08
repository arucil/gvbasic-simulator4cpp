#include "gvb.h"

GC::GC() {
    head = new Null;
    head->pre = head->nxt = head;
}

GC::~GC() {
    gc();
    delete head;
    head = 0;
}

S *GC::link(S *n) { //仅在new的时候link
    n->nxt = head->nxt;
    n->pre = head;
    n->nxt->pre = n;
    head->nxt = n;
    return n;
}

E *GC::link(E *n) {
    n->nxt = head->nxt;
    n->pre = head;
    n->nxt->pre = n;
    head->nxt = n;
    return n;
}

Goto *GC::link(Goto *n) {
    n->nxt = head->nxt;
    n->pre = head;
    n->nxt->pre = n;
    head->nxt = n;
    return n;
}

Gosub *GC::link(Gosub *n) {
    n->nxt = head->nxt;
    n->pre = head;
    n->nxt->pre = n;
    head->nxt = n;
    return n;
}

Id *GC::link(Id *n) {
    n->nxt = head->nxt;
    n->pre = head;
    n->nxt->pre = n;
    head->nxt = n;
    return n;
}

V *GC::link(V *n) {
    n->nxt = head->nxt;
    n->pre = head;
    n->nxt->pre = n;
    head->nxt = n;
    return n;
}

Array *GC::link(Array *n) {
    n->nxt = head->nxt;
    n->pre = head;
    n->nxt->pre = n;
    head->nxt = n;
    return n;
}

Fn *GC::link(Fn *n) {
    n->nxt = head->nxt;
    n->pre = head;
    n->nxt->pre = n;
    head->nxt = n;
    return n;
}

void GC::gc() {
    O *t;
    for (O *i = head->nxt; i != head;) {
        t = i;
        i->pre->nxt = i->nxt;
        i->nxt->pre = i->pre;
        i = i->nxt;
        delete t;
    }
}

void GC::remove(O *e) {
    e->pre->nxt = e->nxt;
    e->nxt->pre = e->pre;
    delete e;
}