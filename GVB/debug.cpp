#include "gvb.h"
#include "debug.h"

using std::endl;

void show(E *e,ostream&out) {
    if(!e)return;
    Binary *b;
    Func *f;
    Access *a;
    CallFn *c;
    switch (e->type) {
    case E::REAL:
        out << ((Real *) e)->rval;
        break;
    case E::STRING:
        out << '"' << ((Str *) e)->sval << '"';
        break;
    case E::BINARY:
        out << "(";
        b=((Binary *)e);
        show(b->l,out);out<<' ';
        if(b->op<128)out<<(char)b->op;else out<<b->op;
        out<<' ';show(b->r,out);
        out<<")";
        break;
    case E::FUNC:f=(Func*)e;
        out<<'['<<f->f<<"](";show(f->x,out);
        if(f->x2) {out<<", ";show(f->x2,out);}
        if(f->x3){out<<", ";show(f->x3,out);}
        out<<')';
        break;
    case E::INKEY:out<<"inkey$";break;
    case E::ID:out<<((Id*)e)->id;break;
    case E::ACCESS:
        a=(Access*)e;
        out<<a->id<<'(';
        for(int i=0;i<a->index.size();i++){
            show(a->index[i],out);out<<", ";
        }
        out<<')';
        break;
    case E::CLFN:
        c=(CallFn*)e;
        out<<"Fn "<<c->fn<<"(";show(c->x,out);out<<")";
        break;
    }
}

static void show(S *s, int lv,ostream&out) {
    Assign*a1;
    If*if1;
    DefFn *df;
    S *s1;
    Print *p;
    For*f4;Dim*d;Draw *s6;Box*s2;Line*s3;Circle*s4;Ellipse*s5;
    Put*s10;Get*s11;Lset*s12;Rset*s13;Field*s14;
    for(int i=0;i<lv;i++)out<<'\t';if(!s)return;
    switch (s->type) {
    case S::CALL:
        out<<"call ";
        show(((Call*)s)->addr,out);
        break;
    case S::ASSIGN:
        a1=(Assign*)s;show(a1->id,out);out<<'=';show(a1->val,out);
        break;
    case S::NONE:
        break;
    case S::GOTO:
        out<<"goto "<<((Goto*)s)->gotos->label;break;
    case S::GOSUB:out<<"gosub "<<((Gosub*)s)->gotos->label;break;
    case S::IF:
        if1=(If*)s;
        out<<"if ";show(if1->con,out);out<<" then\n";
        for(s1 = if1->stm;s1!=if1->next;s1=s1->next){
            show(s1,lv+1,out);out<<endl;
        }
        if(if1->elsestm){
            show(0,lv,out);out<<"else\n";
            for(s1 = if1->elsestm;s1!=if1->next;s1=s1->next){
                show(s1,lv+1,out);out<<endl;
            }
        }
        show(0,lv,out);out<<"endif";
        break;
    case S::DEFFN:
        df=(DefFn*)s;
        out<<"def fn "<<df->f<<"("<<df->x<<")=";show(df->fn,out);
        break;
    case S::PRINT:
        p=(Print*)s;
        out<<"print ";
        for(int i=0;i<p->ps.size();i++) {
            show(p->ps[i],out);if(p->del[i]==Print::CR)out<<",";
            else if(p->del[i]==Print::NO)out<<";";
            out<<' ';
        }break;
    case S::FOR:
        f4=(For*)s;
        out<<"for "<<f4->v<<" to ";show(f4->dest,out);out<<" step ";
        if(f4->step)show(f4->step,out);else out<<"1";break;
    case S::NEXT:
        out<<"next "<<((Next*)s)->v;
        break;
    case S::END:out<<"end";break;
    case S::CLS:out<<"cls";break;
    case S::GRAPH:out<<"graph";break;
    case S::TEXT:out<<"text";break;
    case S::RETURN:out<<"return";break;
    case S::POP:out<<"pop";break;
    case S::CLEAR:out<<"clear";break;
    case S::INVERSE:out<<"inverse";break;
    case S::DIM:
        out<<"dim ";show(((Dim*)s)->id,out);break;
    case S::WHILE:out<<"while ";show(((While*)s)->con,out);break;
    case S::WEND:out<<"wend";break;
    case S::INKEY:out<<"inkey";break;
    case S::FINPUT:{FInput *f=(FInput*)s;
        out<<"input #"<<f->fnum<<", ";
        for(int i=0;i<f->ids.size();i++){show(f->ids[i],out);out<<", ";}}break;
    case S::WRITE:{Write *f=(Write*)s;
        out<<"write #"<<f->fnum<<", ";
        for(int i=0;i<f->es.size();i++){show(f->es[i],out);out<<", ";}}break;
    case S::READ:{Read *f=(Read*)s;
        out<<"read ";
        for(int i=0;i<f->ids.size();i++){show(f->ids[i],out);out<<", ";}}break;
    case S::INPUT:{Input *f=(Input*)s;
        out<<"input \""<<f->prm<<"\", ";
        for(int i=0;i<f->ids.size();i++){show(f->ids[i],out);out<<", ";}}break;
    case S::CLOSE:out<<"close #"<<((Close*)s)->fnum;break;
    case S::OPEN:{Open *o=(Open*)s;
        out<<"open ";show(o->fname,out);out<<" for ";
        out<<(o->mode==Open::MINPUT?"input":o->mode==Open::MOUTPUT?"output":
            o->mode==Open::MRANDOM?"random":"append");
        out<<" as #"<<o->fnum<<" len="<<o->len;}break;
    case S::LOCATE:{Locate*l=(Locate*)s;
        out<<"locate ";show(l->col,out);out<<", ";show(l->row,out);}break;
    case S::POKE:{Poke*l=(Poke*)s;
        out<<"poke ";show(l->addr,out);out<<", ";show(l->val,out);}break;
    case S::SWAP:{Swap*l=(Swap*)s;
        out<<"swap ";show(l->id1,out);out<<", ";show(l->id2,out);}break;
    case S::RESTORE:out<<"restore "<<((Restore*)s)->rlabel;break;
    case S::ON:{On*o=(On*)s;
        out<<"on ";show(o->con,out);out<<(o->isSub?" gosub ":" goto ");
        for(int i=0;i<o->labels.size();i++){out<<o->stms[i]->label;out<<", ";}}break;
    case S::DRAW:s6=(Draw*)s;
        out<<"draw ";show(s6->x,out);out<<", ";show(s6->y,out);out<<", ";show(s6->t,out);
        out<<", ";break;
    case S::BOX:s2=(Box*)s;
        out<<"box ";show(s2->x1,out);out<<", ";show(s2->y1,out);out<<", ";show(s2->x2,out);
        out<<", ";show(s2->y2,out);out<<", ";show(s2->f,out);out<<", ";show(s2->t,out);break;
    case S::LINE:s3=(Line*)s;
        out<<"line ";show(s3->x1,out);out<<", ";show(s3->y1,out);out<<", ";show(s3->x2,out);
        out<<", ";show(s3->y2,out);out<<", ";show(s3->t,out);break;
    case S::CIRCLE:s4=(Circle*)s;
        out<<"circle ";show(s4->x,out);out<<", ";show(s4->y,out);out<<", ";show(s4->r,out);
        out<<", ";show(s4->f,out);out<<", ";show(s4->t,out);break;
    case S::ELLIPSE:s5=(Ellipse*)s;
        out<<"ellipse ";show(s5->x,out);out<<", ";show(s5->y,out);out<<", ";show(s5->xr,out);
        out<<", ";show(s5->yr,out);out<<", ";show(s5->f,out);out<<", ";show(s5->t,out);break;
    case S::LSET:s12=(Lset*)s;
        out<<"lset ";show(s12->id,out);out<<"=";show(s12->s,out);break;
    case S::RSET:s13=(Rset*)s;
        out<<"rset ";show(s13->id,out);out<<"=";show(s13->s,out);break;
    case S::PUT:s10=(Put*)s;
        out<<"put #"<<s10->fnum<<", ";show(s10->rec,out);break;
    case S::GET:s11=(Get*)s;
        out<<"get #"<<s11->fnum<<", ";show(s11->rec,out);break;
    case S::FIELD:s14=(Field*)s;
        out<<"field #"<<s14->fnum<<", ";for(int i=0;i<s14->bys.size();i++){
            out<<s14->bys[i]<<" as "<<s14->ids[i]<<", ";}break;
    case S::SLEEP:
        out<<"sleep ";show(((SSleep*)s)->ms,out);break;
    case S::PAINT:{SPaint *sp=(SPaint*)s;
        out<<"paint ";show(sp->addr,out);out<<", ";show(sp->x,out);out<<", ";
        show(sp->y, out);out<<", ";show(sp->w,out);out<<", ";show(sp->h,out);out<<", ";
        show(sp->mode,out);
                  }break;
    case S::LOAD:{SLoad*sl=(SLoad*)s;
        out<<"load ";show(sl->addr,out);for(int i=0;i<sl->values.size();i++)
            out<<", "<<(int)sl->values[i];
                 }break;
    case S::FPUTC:{SFputc *sfp=(SFputc*)s;
        out<<"fputc #"<<sfp->fnum;out<<", ";show(sfp->ch,out);
                  }break;
    case S::FSEEK:{SFseek *sfs=(SFseek*)s;
        out<<"fseek #"<<sfs->fnum;out<<", ";show(sfs->pt,out);
                  }break;
    case S::FREAD:{SFread*sfr=(SFread*)s;
        out<<"fread #"<<sfr->fnum;show(sfr->addr,out);out<<", ";show(sfr->size,out);
                  }break;
    case S::FWRITE:{SFwrite*sfw=(SFwrite*)s;
        out<<"fwrite #"<<sfw->fnum;show(sfw->addr,out);out<<", ";show(sfw->size,out);
                   }break;
    }
}

void show(S *s, ostream &out) {
    while (s) {
        out<<s->label<<' ';
        show(s, 0, out);out<<endl;
        s=s->next;
    }
}

void show(GVB &p, ostream &out) {
    out<<"********************----------------vars:"<<p.vars.size()<<endl;
    for(GVB::variter i = p.vars.begin(); i != p.vars.end(); i++) {
            const IdKey &j = i->first;
            V *v = i->second;
            out << j.id << '[' << E::typeDesc(v->vtype) << ']';
            if (j.type == V::ARRAY) {
                out<<'(';Array *a=(Array*)v;
                for(int i=0;i<a->bounds.size();i++){
                    out<<a->bounds[i]<<',';
                }out<<"): [";
                switch(a->vtype){case E::VINT:if(a->ivals.size()<40)
                for(int i=0;i<a->ivals.size();i++)out<<a->ivals[i]<<", ";else
                    out<<a->ivals.size();
                break;
                case E::VREAL:if(a->rvals.size()<40)
                    for(int i=0;i<a->rvals.size();i++)out<<a->rvals[i]<<", ";else
                    out<<a->rvals.size();break;
                case E::VSTRING:if(a->svals.size()<40)
                    for(int i=0;i<a->svals.size();i++)out<<'"'<<a->svals[i]<<"\", ";else
                    out<<a->svals.size();break;
                }out<<"]";
            }else {out<<": ";switch(v->vtype){
            case E::VINT:out<<v->ival;break;
            case E::VREAL:out<<v->rval;break;
            case E::VSTRING:out<<'"'<<v->sval<<'"';break;
            }
            }
            out<<endl;
        }
        out<<"-----------------------------------data:\n"<<p.data.size()<<": ";
        p.data.restore();
        for (int i=0;i<p.data.size();i++) {
            out<<'"'<<p.data.get()<<"\", ";
        }
        out<<"\n-----------------------------------"
            "\nloops:"<<p.loops.size()<<"\nsubs:"<<p.subs.size()<<endl;
}