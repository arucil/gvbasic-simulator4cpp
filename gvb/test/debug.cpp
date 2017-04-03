#include "debug.h"
#include <iostream>
#include <cassert>
#include "../tree.h"
#include "../lex.h"

using namespace std;
using namespace gvbsim;

namespace {

#define f(s)  case Func::Type::s: return #s
#define fs(s) case Func::Type::s: return #s "$"

string getFuncName(Func::Type t) {
   switch (t) {
   f(ABS); f(ASC); f(ATN); f(COS); f(EXP); f(INT); f(LEN); f(LOF);
   f(LOG); f(PEEK); f(POS); f(RND); f(SGN); f(SIN); f(SQR); f(TAN); f(VAL);
   fs(CHR); fs(CVI); fs(CVS); fs(LEFT); fs(MID); fs(MKI); fs(MKS); fs(RIGHT);
   fs(STR); f(TAB); f(SPC); f(NOT);
   case Func::Type::FEOF: return "EOF";
   case Func::Type::NEG: return "-";
   default:
      assert(0);
   }
}

#undef fs
#undef f

void print(Expr *e, ostream &out) {
   switch (e->type) {
   case Expr::Type::REAL:
      out.precision(9);
      out << ((Real *) e)->rval;
      break;
   case Expr::Type::STRING:
      out << '"' << ((Str *) e)->sval << '"';
      break;
   case Expr::Type::BINARY: {
      out << '(';
      Binary *b = ((Binary *) e);
      print(b->left, out);
      out << ' ';
      if (b->op < 128)
         out << (char) b->op;
      else
         out << Token::toString(b->op);
      out << ' ';
      print(b->right, out);
      out << ')';
      break;
   }
   case Expr::Type::FUNCCALL: {
      FuncCall *f = (FuncCall *) e;
      out << getFuncName(f->ftype) << '(';
      print(f->expr1, out);
      if (f->expr2) {
         out << ", ";
         print(f->expr2, out);
         if (f->expr3) {
            out << ", ";
            print(f->expr3, out);
         }
      }
      out << ')';
      break;
   }
   case Expr::Type::INKEY:
      out << "INKEY$";
      break;
   case Expr::Type::ID:
      out << ((Id *) e)->id;
      break;
   case Expr::Type::ARRAYACCESS: {
      ArrayAccess *a = (ArrayAccess *) e;
      out << a->id << '[';
      for (int i = 0; i < a->indices.size(); ++i) {
         print(a->indices[i], out);
         if (i < a->indices.size() - 1)
            out << ", ";
      }
      out << ']';
      break;
   }
   case Expr::Type::USERCALL: {
      UserCall *c = (UserCall *) e;
      out << "FN " << c->fnName << "(";
      print(c->expr, out);
      out << ')';
      break;
   }
   default:
      assert(0);
   }
}

#define INDENT(t)  for(int _=0;_<t;++_)cout<<"    ";

void print(Stmt *s, int lv, ostream &out) {
   INDENT(lv);
   switch (s->type) {
   case Stmt::Type::CALL:
      out << "CALL ";
      print(((Call *) s)->addr, out);
      break;
   case Stmt::Type::ASSIGN:
      out << "LET ";
      print(((Assign *) s)->id, out);
      out << " = ";
      print(((Assign *) s)->val, out);
      break;
   case Stmt::Type::NONE:
      out << "NONE";
      break;
   case Stmt::Type::GOTO:
   case Stmt::Type::GOSUB: {
      Goto *g = (Goto *) s;
      out << (g->type == Stmt::Type::GOSUB? "GOSUB " : "GOTO ");
      out << g->stm->label;
      break;
   }
   case Stmt::Type::DEFFN: {
      DefFn *d = (DefFn *) s;
      out << "DEF FN " << d->fnName << "(" << d->varName << ") = ";
      print(d->fn, out);
      break;
   }
   case Stmt::Type::PRINT: {
      Print *p = (Print *) s;
      out << "PRINT ";
      for (auto i : p->exprs) {
         print(i.first, out);
         if (Print::Delimiter::CR == i.second)
            out << ", ";
         else
            out << "; ";
      }
      break;
   }
   case Stmt::Type::IF: {
      If *i = (If *) s;
      out << "IF ";
      print(i->cond, out);
      out << " THEN\n";
      for (Stmt *s1 = i->stmThen; s1 != i->next; s1 = s1->next) {
         print(s1, lv + 1, out);
         out << endl;
      }
      if (i->stmElse) {
         INDENT(lv);
         out << "ELSE\n";
         for (Stmt *s1 = i->stmElse; s1 != i->next; s1 = s1->next) {
            print(s1, lv + 1, out);
            out << endl;
         }
      }
      INDENT(lv);
      out << "ENDIF";
      break;
   }
   case Stmt::Type::FOR: {
      For *f = (For *) s;
      out << "FOR " << f->var << " TO ";
      print(f->dest, out);
      if (f->step) {
         out << " STEP ";
         print(f->step, out);
      }
      break;
   }
   case Stmt::Type::NEXT: {
      Next *n = (Next *) s;
      if (n->var.empty())
         out << "NEXT";
      else
         out << "NEXT " << n->var;
      break;
   }
   case Stmt::Type::END:out<<"END";break;
   case Stmt::Type::CLS:out<<"CLS";break;
   case Stmt::Type::GRAPH:out<<"GRAPH";break;
   case Stmt::Type::TEXT:out<<"TEXT";break;
   case Stmt::Type::RETURN:out<<"RETURN";break;
   case Stmt::Type::POP:out<<"POP";break;
   case Stmt::Type::CLEAR:out<<"CLEAR";break;
   case Stmt::Type::INVERSE:out<<"INVERSE";break;
   case Stmt::Type::DIM:
      out << "DIM ";
      print(((Dim *) s)->id, out);
      break;
   case Stmt::Type::WHILE:
      out << "WHILE ";
      print(((While *) s)->cond, out);
      break;
   case Stmt::Type::WEND: out << "WEND"; break;
   case Stmt::Type::INKEY: out << "INKEY$"; break;
   case Stmt::Type::FINPUT: {
      FInput *f = (FInput *) s;
      out << "INPUT #" << f->fnum + 1 << ", ";
      for (int i = 0; i < f->ids.size(); ++i) {
         print(f->ids[i], out);
         if (i < f->ids.size() - 1)
            out << ", ";
      }
      break;
   }
   case Stmt::Type::INPUT: {
      Input *f = (Input *) s;
      out << "INPUT ";
      if (f->prompt.size()) {
         out << '"' << f->prompt << "\", ";
      }
      for (int i = 0; i < f->ids.size(); ++i) {
         print(f->ids[i], out);
         if (i < f->ids.size() - 1)
            out << ", ";
      }
      break;
   }
   case Stmt::Type::WRITE: {
      Write *f = (Write *) s;
      out << "WRITE #" << f->fnum + 1 << ", ";
      for (int i = 0; i < f->exprs.size(); ++i) {
         print(f->exprs[i], out);
         if (i < f->exprs.size() - 1)
            out << ", ";
      }
      break;
   }
   case Stmt::Type::READ: {
      Read *f = (Read *) s;
      out << "READ ";
      for (int i = 0; i < f->ids.size(); ++i) {
         print(f->ids[i], out);
         if (i < f->ids.size() - 1)
            out << ", ";
      }
      break;
   }
   case Stmt::Type::CLOSE:
      out << "CLOSE #" << ((Close *) s)->fnum + 1;
   case Stmt::Type::OPEN: {
      Open *o = (Open *) s;
      out << "OPEN ";
      print(o->fname, out);
      out << " FOR ";
      out << (Open::Mode::INPUT == o->mode ? "INPUT" :
              Open::Mode::OUTPUT == o->mode ? "OUTPUT" :
              Open::Mode::APPEND == o->mode ? "APPEND" : "RANDOM");
      out << " AS #" << o->fnum + 1;
      if (o->len != Open::NOLEN)
         out << " LEN=" << o->len;
      break;
   }
   case Stmt::Type::LOCATE: {
      Locate *l = (Locate *) s;
      out << "LOCATE ";
      if (l->row)
         print(l->row, out);
      out << ", ";
      print(l->col, out);
      break;
   }
   case Stmt::Type::POKE: {
      Poke *l = (Poke *) s;
      out << "POKE ";
      print(l->addr, out);
      out << ", ";
      print(l->val, out);
      break;
   }
   case Stmt::Type::SWAP: {
      Swap *l = (Swap *) s;
      out << "SWAP ";
      print(l->id1, out);
      out << ", ";
      print(l->id2, out);
      break;
   }
   case Stmt::Type::RESTORE: {
      Restore *r = (Restore *) s;
      out << "RESTORE";
      if (r->label != Restore::NO_LABEL)
         out << " " << r->label;
      break;
   }
   case Stmt::Type::ON: {
      On *o = (On *) s;
      out << "ON ";
      print(o->cond, out);
      out << (o->isSub ? " GOSUB" : " GOTO");
      for (int i = 0; i < o->addrs.size(); ++i) {
         out << o->addrs[i].stm->label;
         if (i < o->addrs.size() - 1)
            out << ", ";
      }
      break;
   }
   case Stmt::Type::DRAW: {
      Draw *d = (Draw *) s;
      out << "DRAW ";
      print(d->x, out);
      out << ", ";
      print(d->y, out);
      if (d->ctype) {
         out << ", ";
         print(d->ctype, out);
      }
      break;
   }
   case Stmt::Type::LINE: {
      Line *l = (Line *) s;
      out << "LINE ";
      print(l->x1, out);
      out << ", ";
      print(l->y1, out);
      out << ", ";
      print(l->x2, out);
      out << ", ";
      print(l->y2, out);
      if (l->ctype) {
         out << ", ";
         print(l->ctype, out);
      }
      break;
   }
   case Stmt::Type::BOX: {
      Box *l = (Box *) s;
      out << "BOX ";
      print(l->x1, out);
      out << ", ";
      print(l->y1, out);
      out << ", ";
      print(l->x2, out);
      out << ", ";
      print(l->y2, out);
      if (l->fill) {
         out << ", ";
         print(l->fill, out);
         if (l->ctype) {
            out << ", ";
            print(l->ctype, out);
         }
      }
      break;
   }
   case Stmt::Type::CIRCLE: {
      Circle *c = (Circle *) s;
      out << "CIRCLE ";
      print(c->x, out);
      out << ", ";
      print(c->y, out);
      out << ", ";
      print(c->radius, out);
      if (c->fill) {
         out << ", ";
         print(c->fill, out);
         if (c->ctype) {
            out << ", ";
            print(c->ctype, out);
         }
      }
      break;
   }
   case Stmt::Type::ELLIPSE: {
      Ellipse *c = (Ellipse *) s;
      out << "ELLIPSE ";
      print(c->x, out);
      out << ", ";
      print(c->y, out);
      out << ", ";
      print(c->rx, out);
      out << ", ";
      print(c->ry, out);
      if (c->fill) {
         out << ", ";
         print(c->fill, out);
         if (c->ctype) {
            out << ", ";
            print(c->ctype, out);
         }
      }
      break;
   }
   case Stmt::Type::LSET:
   case Stmt::Type::RSET: {
      LRSet *l = (LRSet *) s;
      if (s->type == Stmt::Type::LSET)
         out << "LSET ";
      else out << "RSET ";
      out << l->id << " = ";
      print(l->str, out);
      break;
   }
   case Stmt::Type::PUT:
   case Stmt::Type::GET: {
      GetPut *g = (GetPut *) s;
      if (Stmt::Type::GET == s->type)
         out << "GET #";
      else out << "PUT #";
      out << g->fnum + 1 << ", ";
      print(g->record, out);
      break;
   }
   case Stmt::Type::FIELD: {
      Field *f = (Field *) s;
      out << "FIELD #" << f->fnum + 1 << ", ";
      for (int i = 0; i < f->fields.size(); ++i) {
         out << f->fields[i].first << " AS " << f->fields[i].second;
         if (i < f->fields.size() - 1)
            out << ", ";
      }
      break;
   }
   case Stmt::Type::SLEEP:
      out << "SLEEP " << ((XSleep *) s)->ticks;
      break;
   case Stmt::Type::PLAY:
      out << "PLAY ";
      print(((Play *) s)->str, out);
      break;
   case Stmt::Type::BEEP:
      out << "BEEP";
      break;
   case Stmt::Type::NEWLINE: {
      NewLine *n = (NewLine *) s;
      out << n->label << " (line:" << n->line << ")";
      break;
   }
   default:
      assert(0);
   }
}

}

void gvbsim::printTree(Stmt *s, ostream &out) {
   while (s) {
      print(s, 0, out);
      out << endl;
      s = s->next;
   }
}
