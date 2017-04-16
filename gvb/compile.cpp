#include "compile.h"
#include <cassert>
#include "gvb.h"
#include "tree.h"
#include "real.h"

using namespace gvbsim;
using namespace std;

#define cerror(...)  GVB::error(m_line, m_label, __VA_ARGS__)


#define f(s)  { #s, Func::Type::s }
#define fs(s) { #s "$", Func::Type::s }

const unordered_map<string, Func::Type> Compiler::s_builtinFuncs = {
      f(ABS), f(ASC), f(ATN), f(COS), f(EXP), f(INT), f(LEN), f(LOF),
      f(LOG), f(PEEK), f(POS), f(RND), f(SGN), f(SIN), f(SQR), f(TAN), f(VAL),
      fs(CHR), fs(CVI), fs(CVS), fs(LEFT), fs(MID), fs(MKI), fs(MKS), fs(RIGHT),
      fs(STR), { "EOF", Func::Type::FEOF },
      f(POINT), f(CHECKKEY), f(FOPEN), f(FGETC), f(FTELL)
};

#undef fs
#undef f


Compiler::Compiler(FILE *fp, NodeManager &nm, DataManager &dm)
      : m_l(fp), m_nodeMan(nm), m_dataMan(dm) {
}

void Compiler::peek() {
   try {
      m_tok = m_l.getToken();
   } catch (Lexer::NumberFormatError) {
      cerror("Number format error");
   }
}

void Compiler::match(int t) {
   if (t != m_tok)
      cerror("Token error: [%c], expecting [%c]", m_tok, t);
   peek();
}

Stmt *Compiler::compile() {
   m_nodeMan.clear();
   m_dataMan.clear();
   m_line = 1;
   m_label = -1;

   peek();

   Stmt *prev;
   NewLine *head = nullptr;

   while (m_tok != -1) {
      NewLine *stmts = aLine(); //不可能为null
      if (m_tok != 10 && m_tok != -1)
         cerror("Token error: [%c], expecting EOL / EOF", m_tok);
      peek();
      if (nullptr == head) {
         prev = head = stmts;
      } else {
         linkStmt(prev, stmts);
      }
      prev = getStmtTail(prev);
      m_line++;
   }
   resolveRefs();

   return translate(head);
}

inline void Compiler::resolveRefs() { //解决label引用
   for (const auto &t : m_refs) {
      Stmt *s;
      int line;
      int label;
      tie(s, line, label) = t;

      switch (s->type) {
      case Stmt::Type::GOSUB:
      case Stmt::Type::GOTO: {
         Goto *g1 = static_cast<Goto *>(s);
         auto i = m_labels.find(g1->label);
         if (m_labels.end() == i) {
            m_line = line;
            m_label = label;
            if (Stmt::Type::GOSUB == s->type) {
               cerror("Label not exist in GOSUB: [%i]", g1->label);
            } else {
               cerror("Label not exist in GOTO: [%i]", g1->label);
            }
         }
         g1->stm = i->second;
         break;
      }
      case Stmt::Type::RESTORE: {
         Restore *r = static_cast<Restore *>(s);
         if (!m_labels.count(r->label)) {
            m_line = line;
            m_label = label;
            cerror("Label not exist in RESTORE: [%i]", r->label);
         }
         break;
      }
      case Stmt::Type ::ON:
         for (auto &i : static_cast<On *>(s)->addrs) {
            auto j = m_labels.find(i.label);
            if (m_labels.end() == j) {
               m_line = line;
               m_label = label;
               cerror("Label not exist in ON: [%i]", i.label);
            }
            i.stm = j->second;
         }
         break;
      default:
         assert(0);
      }
   }
}

inline NewLine *Compiler::aLine() {
   int t = m_l.ival;
   match(Token::INT);
   if (t <= m_label)
      cerror("Invalid label");
   m_label = t;
   if (m_labels.count(m_label))
      cerror("Label duplicate");
   m_dataMan.addLabel(m_label);

   NewLine *s = m_nodeMan.make<NewLine>(m_label, m_line);
   s->next = stmts();
   return m_labels[m_label] = s; //记录一行的第一条语句
}

inline Stmt *Compiler::stmts() { //可能为null
   Stmt *head = nullptr, *cur;

   while (true) {
      Stmt *stm = stmt(false);//可能为null
      if (stm) {
         if (!head) {
            head = cur = stm;
         } else {
            linkStmt(cur, stm);
         }
         cur = getStmtTail(cur);
      }
      if (m_tok == 10 || m_tok == -1)
         break;
      match(':');
   }
   return head;
}

inline Stmt *Compiler::getStmtTail(Stmt *s) {
   if (!s) {
      return nullptr;
   } else {
      while (s->next)
         s = s->next;
      return s;
   }
}

void Compiler::linkStmt(Stmt *s, Stmt *next) {
   if (s) {
      s->next = next;
      if (Stmt::Type::IF == s->type) {
         If *i = static_cast<If *>(s);
         linkStmt(getStmtTail(i->stmThen), next);
         linkStmt(getStmtTail(i->stmElse), next);
      }
   }
}

Stmt *Compiler::stmt(bool inIf) { // 可能为null
   while (true) {
      switch (m_tok) {
      case Token::REM: {
         int c;
         while ((c = m_l.getc()) != 10 && c != -1)
            ;
         m_tok = c;
         return nullptr;
      }
      case Token::END:
         peek();
         return m_nodeMan.make<Stmt>(Stmt::Type::END);

      case Token::CLS:
         peek();
         return m_nodeMan.make<Stmt>(Stmt::Type::CLS);

      case Token::GRAPH:
         peek();
         return m_nodeMan.make<Stmt>(Stmt::Type::GRAPH);

      case Token::TEXT:
         peek();
         return m_nodeMan.make<Stmt>(Stmt::Type::TEXT);

      case Token::RETURN:
         peek();
         return m_nodeMan.make<Stmt>(Stmt::Type::RETURN);

      case Token::POP:
         peek();
         return m_nodeMan.make<Stmt>(Stmt::Type::POP);

      case Token::CLEAR:
         peek();
         return m_nodeMan.make<Stmt>(Stmt::Type::CLEAR);

      case Token::INVERSE:
         peek();
         return m_nodeMan.make<Stmt>(Stmt::Type::INVERSE);

      case Token::CONT:
         peek();
         return nullptr;

      case Token::BEEP:
         peek();
         return m_nodeMan.make<Stmt>(Stmt::Type::BEEP);

      case Token::PLAY:
         peek();
         return m_nodeMan.make<Play>(expr(Value::Type::STRING));

      case ':':
         peek();
         continue;

      case Token::INKEY:
         peek();
         return m_nodeMan.make<Stmt>(Stmt::Type::INKEY);

      case Token::LET:
         peek();
      case Token::ID: { // 赋值
         Id *id1 = getId();
         match('=');
         Expr *e1 = expr(getRValType(id1->vtype));
         return m_nodeMan.make<Assign>(id1, e1);
      }

      case Token::CALL: {
         peek();
         Expr *e1 = expr(Value::Type::REAL);
         return m_nodeMan.make<Call>(e1);
      }

      case Token::GOTO:
      case Token::GOSUB: {
         bool isSub = Token::GOSUB == m_tok;
         peek();
         int i1 = m_l.ival;
         match(Token::INT);
         return findLabel(m_nodeMan.make<Goto>(i1, isSub));
      }

      case Token::IF:
         return ifstmt();

      case Token::DATA:
         datastmt();
         return nullptr;

      case Token::DEF:
         return defstmt();

      case Token::PRINT:
         return printstmt();

      case Token::FOR:
         return forstmt();

      case Token::NEXT:
         return nextstmt();

      case Token::DIM:
         return dimstmt();

      case Token::WHILE:
         peek();
         return m_nodeMan.make<While>(expr(Value::Type::REAL));

      case Token::WEND:
         peek();
         return m_nodeMan.make<Stmt>(Stmt::Type::WEND);

      case Token::INPUT:
         return inputstmt();

      case Token::WRITE:
         return writestmt();

      case Token::READ:
         return readstmt();

      case Token::CLOSE:
         peek();
         return m_nodeMan.make<Close>(getFileNum());

      case Token::OPEN:
         return openstmt();

      case Token::LOCATE: {
         peek();
         Expr *e1 = ',' == m_tok ? nullptr : expr(Value::Type::REAL);
         match(',');
         return m_nodeMan.make<Locate>(e1, expr(Value::Type::REAL));
      }

      case Token::POKE: {
         peek();
         Expr *e1 = expr(Value::Type::REAL);
         match(',');
         return m_nodeMan.make<Poke>(e1, expr(Value::Type::REAL));
      }

      case Token::SWAP: {
         peek();
         Id *id1 = getId();
         match(',');
         Id *id2 = getId();
         if (id1->vtype != id2->vtype) {
            cerror("Incompatible type in SWAP: [%t, %t]",
                   id1->vtype, id2->vtype);
         }
         return m_nodeMan.make<Swap>(id1, id2);
      }

      case Token::RESTORE: {
         peek();
         int label;
         if (Token::INT == m_tok) {
            label = m_l.ival;
            peek();
         } else
            label = Restore::NO_LABEL;
         return findLabel(m_nodeMan.make<Restore>(label));
      }

      case Token::ON:
         return onstmt();

      case Token::DRAW: {
         peek();
         Expr *e1 = expr(Value::Type::REAL);
         match(',');
         Expr *e2 = expr(Value::Type::REAL);
         Expr *e3 = nullptr;
         if (m_tok == ',') {
            peek();
            e3 = expr(Value::Type::REAL);
         }
         return m_nodeMan.make<Draw>(e1, e2, e3);
      }

      case Token::LINE: {
         peek();
         Expr *e1 = expr(Value::Type::REAL);
         match(',');
         Expr *e2 = expr(Value::Type::REAL);
         match(',');
         Expr *e3 = expr(Value::Type::REAL);
         match(',');
         Expr *e4 = expr(Value::Type::REAL);
         Expr *e5 = nullptr;
         if (m_tok == ',') {
            peek();
            e5 = expr(Value::Type::REAL);
         }
         return m_nodeMan.make<Line>(e1, e2, e3, e4, e5);
      }

      case Token::BOX: {
         peek();
         Expr *e1 = expr(Value::Type::REAL);
         match(',');
         Expr *e2 = expr(Value::Type::REAL);
         match(',');
         Expr *e3 = expr(Value::Type::REAL);
         match(',');
         Expr *e4 = expr(Value::Type::REAL);
         Expr *e5 = nullptr, *e6 = nullptr;
         if (m_tok == ',') {
            peek();
            e5 = expr(Value::Type::REAL);
            if (m_tok == ',') {
               peek();
               e6 = expr(Value::Type::REAL);
            }
         }
         return m_nodeMan.make<Box>(e1, e2, e3, e4, e5, e6);
      }

      case Token::CIRCLE: {
         peek();
         Expr *e1 = expr(Value::Type::REAL);
         match(',');
         Expr *e2 = expr(Value::Type::REAL);
         match(',');
         Expr *e3 = expr(Value::Type::REAL);
         Expr *e4 = nullptr, *e5 = nullptr;
         if (m_tok == ',') {
            peek();
            e4 = expr(Value::Type::REAL);
            if (m_tok == ',') {
               peek();
               e5 = expr(Value::Type::REAL);
            }
         }
         return m_nodeMan.make<Circle>(e1, e2, e3, e4, e5);
      }

      case Token::ELLIPSE: {
         peek();
         Expr *e1 = expr(Value::Type::REAL);
         match(',');
         Expr *e2 = expr(Value::Type::REAL);
         match(',');
         Expr *e3 = expr(Value::Type::REAL);
         match(',');
         Expr *e4 = expr(Value::Type::REAL);
         Expr *e5 = nullptr, *e6 = nullptr;
         if (m_tok == ',') {
            peek();
            e5 = expr(Value::Type::REAL);
            if (m_tok == ',') {
               peek();
               e6 = expr(Value::Type::REAL);
            }
         }
         return m_nodeMan.make<Ellipse>(e1, e2, e3, e4, e5, e6);
      }

      case Token::LSET:
      case Token::RSET: {
         bool isL = m_tok == Token::LSET;
         peek();
         Id *id1 = getId();
         if (id1->vtype != Value::Type::STRING) {
            cerror("Need string Id in LSET: [%s]", id1->id);
         }
         match('=');
         return m_nodeMan.make<LRSet>(id1, expr(Value::Type::STRING), isL);
      }

      case Token::GET:
      case Token::PUT: {
         bool isGet = Token::GET == m_tok;
         peek();
         int fnum = getFileNum();
         match(',');
         return m_nodeMan.make<GetPut>(fnum, expr(Value::Type::REAL), isGet);
      }

      case Token::FIELD:
         return fieldstmt();

      case Token::INT:
         if (inIf) {
            int label = m_l.ival;
            peek();
            return findLabel(m_nodeMan.make<Goto>(label, false));
         }
         cerror("Token error: [%c], statement expected", m_tok);

      case Token::SLEEP:
         peek();
         return m_nodeMan.make<XSleep>(expr(Value::Type::REAL));

      case Token::PAINT: {
         peek();
         Expr *addr = expr(Value::Type::REAL);
         match(',');
         Expr *x = expr(Value::Type::REAL);
         match(',');
         Expr *y = expr(Value::Type::REAL);
         match(',');
         Expr *w = expr(Value::Type::REAL);
         match(',');
         Expr *h = expr(Value::Type::REAL);
         Expr *mode;
         if (',' == m_tok) {
            peek();
            mode = expr(Value::Type::REAL);
         } else {
            mode = nullptr;
         }
         return m_nodeMan.make<XPaint>(addr, x, y, w, h, mode);
      }

      case Token::LOAD: {
         peek();
         auto l1 = m_nodeMan.make<XLoad>(expr(Value::Type::REAL));
         while (true) {
            match(',');
            if (m_tok != Token::INT) {
               match(Token::INT);
            }
            if (m_l.ival > 255) {
               cerror("Integer overflow: %i", m_l.ival);
            }
            l1->values.push_back(static_cast<uint8_t>(m_l.ival));
            peek();
            if (',' != m_tok)
               break;
         }
         return l1;
      }

      case Token::FSEEK: {
         peek();
         int fnum = getFileNum();
         match(',');
         return m_nodeMan.make<XFseek>(fnum, expr(Value::Type::REAL));
      }

      case Token::FPUTC: {
         peek();
         int fnum = getFileNum();
         match(',');
         return m_nodeMan.make<XFputc>(fnum, expr(Value::Type::STRING));
      }

      case Token::FREAD:
      case Token::FWRITE: {
         bool isWrite = m_tok == Token::FWRITE;
         peek();
         int fnum = getFileNum();
         match(',');
         Expr *addr = expr(Value::Type::REAL);
         match(',');
         return m_nodeMan.make<XFrw>(fnum, addr, expr(Value::Type::REAL), isWrite);
      }

      default: // eol, eof, else ...
         return nullptr;
      }
   }
}

inline Stmt *Compiler::ifstmt() {
   peek();
   Expr *e1 = expr(Value::Type::REAL);
   Stmt *stmThen, *stmElse = nullptr;

   if (Token::GOTO == m_tok) { // if ... goto xxx
      peek();
      int i1 = m_l.ival;
      match(Token::INT);
      stmThen = findLabel(m_nodeMan.make<Goto>(i1, false));
      if (':' == m_tok)
         peek();
   } else { // if ... then ...
      match(Token::THEN);
      stmThen = ifclause();
   }

   if (Token::ELSE == m_tok) {
      peek();
      stmElse = ifclause();
   }

   return m_nodeMan.make<If>(e1, stmThen, stmElse);
}

inline Stmt *Compiler::ifclause() {
   Stmt *head = nullptr, *cur;

   while (true) {
      Stmt *stm = stmt(true);//可能为null
      if (stm) {
         if (!head) {
            head = cur = stm;
         } else {
            linkStmt(cur, stm);
         }
         cur = getStmtTail(cur);
      }
      if (m_tok == 10 || m_tok == Token::ELSE || m_tok == -1)
         break;
      match(':');
   }
   if (!head) {
      return m_nodeMan.make<Stmt>(Stmt::Type::NONE);
   }
   return head;
}

inline void Compiler::datastmt() {
   while (true) {
      string s;
      int i1;

      m_l.skipSpace();
      if ((i1 = m_l.getc()) == '"') {
         while ((i1 = m_l.getc()) != '"' && i1 != 13 && i1 != 10 && i1 != -1) {
            s += static_cast<char>(i1);
         }
         if (i1 != '"') {
            m_tok = i1 == 13 ? m_l.getc() /* assuming 10 */ : i1;
         } else {
            i1 = m_l.getc();
         }
      } else {
         while (i1 != ',' && i1 != 13 && i1 != ':' && i1 != 10 && i1 != -1) {
            s += static_cast<char>(i1);
            i1 = m_l.getc();
         }
      }
      m_dataMan.add(addCNPrefix(rtrim(s)));
      if (i1 != ',') {
         m_tok = i1 == 13 ? m_l.getc() /* assuming 10 */ : i1;
         break;
      }
   }
}

inline Stmt *Compiler::defstmt() {
   peek();
   match(Token::FN);
   string f = m_l.sval;
   match(Token::ID);
   match('(');
   string var = m_l.sval;
   match(Token::ID);
   match(')');
   match('=');

   Expr *fn = expr(getIdRValType(f));

   string newVar = string("_") + getTypeSuffix(var);
   translateUserCall(fn, var, newVar);

   return m_nodeMan.make<DefFn>(f, newVar, fn);
}

// 将def fn函数自变量替换成另一个变量
void Compiler::translateUserCall(Expr *e1, const string &s0, const string &s1) {
   switch (e1->type) {
   case Expr::Type::BINARY:
      return translateUserCall(static_cast<Binary *>(e1)->left, s0, s1),
            translateUserCall(static_cast<Binary *>(e1)->right, s0, s1);
   case Expr::Type::ARRAYACCESS:
      for (Expr *i : static_cast<ArrayAccess *>(e1)->indices)
         translateUserCall(i, s0, s1);
      return;
   case Expr::Type::FUNCCALL: {
      auto fc1 = static_cast<FuncCall *>(e1);
      translateUserCall(fc1->expr1, s0, s1);
      if (fc1->expr2) {
         translateUserCall(fc1->expr2, s0, s1);
         if (fc1->expr3)
            translateUserCall(fc1->expr3, s0, s1);
      }
      return;
   }
   case Expr::Type::ID:
      if (static_cast<Id *>(e1)->id == s0)
         static_cast<Id *>(e1)->id = s1;
      return;
   case Expr::Type::USERCALL:
      translateUserCall(static_cast<UserCall *>(e1)->expr, s0, s1);
      return;
   default:
      return;
   }
}

inline Stmt *Compiler::printstmt() {
   peek();
   Print *p1 = m_nodeMan.make<Print>();

   while (m_tok != ':' && m_tok != 10 && m_tok != -1 && m_tok != Token::ELSE) {
      if (m_tok != ';' && m_tok != ',') {
         switch (m_tok) {
         case Token::SPC:
         case Token::TAB: {
            int t = m_tok;
            peek();
            match('(');
            Expr *e1 = expr(Value::Type::REAL);
            match(')');
            p1->exprs.push_back(
                  make_pair(m_nodeMan.make<FuncCall>(
                           Token::SPC == t ? Func::Type::SPC : Func::Type::TAB,
                           Value::Type::STRING, e1),
                        Print::Delimiter::NO));
            break;
         }
         default:
            p1->exprs.push_back(make_pair(expr(), Print::Delimiter::NO));
            break;
         }
      } else {
         p1->exprs.push_back(make_pair<Expr *>(nullptr, Print::Delimiter::NO));
      }
      if (',' == m_tok || ':' == m_tok || 10 == m_tok || -1 == m_tok
            || Token::ELSE == m_tok)
         p1->exprs.back().second = Print::Delimiter::CR;
      if (m_tok == ';' || m_tok == ',')
         peek();
   }
   if (p1->exprs.empty()) {
      p1->exprs.push_back(make_pair<Expr *>(nullptr, Print::Delimiter::CR));
   }
   return p1;
}

inline Stmt *Compiler::forstmt() {
   peek();
   string var = m_l.sval;
   match(Token::ID);
   Value::Type vt;
   if ((vt = getIdType(var)) == Value::Type::STRING) {
      cerror("Incompatible Id type in FOR: [%s]", var);
   }
   match('=');
   Stmt *s1 = m_nodeMan.make<Assign>(m_nodeMan.make<Id>(var, vt),
                                     expr(Value::Type::REAL));
   match(Token::TO);
   Expr *dest = expr(Value::Type::REAL);
   Expr *step = nullptr;
   if (m_tok == Token::STEP) {
      peek();
      step = expr(Value::Type::REAL);
   }
   s1->next = m_nodeMan.make<For>(var, dest, step);
   return s1;
}

inline Stmt *Compiler::nextstmt() {
   peek();
   if (m_tok != Token::ID) {
      return m_nodeMan.make<Next>("");
   }
   Stmt *head = nullptr, *cur;
   while (true) {
      string id = m_l.sval;
      match(Token::ID);
      if (getIdType(id) == Value::Type::STRING) {
         cerror("Incompatible Id type in NEXT: [%s]", id);
      }
      Stmt *next = m_nodeMan.make<Next>(id);
      if (!head) {
         head = cur = next;
      } else {
         cur->next = next;
         cur = next;
      }
      if (m_tok != ',')
         break;
      peek();
   }
   return head;
}

inline Stmt *Compiler::dimstmt() {
   peek();
   Stmt *head = nullptr, *cur;
   while (true) {
      Stmt *next = m_nodeMan.make<Dim>(getId());
      if (!head) {
         cur = head = next;
      } else {
         cur->next = next;
         cur = next;
      }
      if (m_tok != ',')
         break;
      peek();
   }
   return head;
}

inline Stmt *Compiler::inputstmt() {
   peek();
   if (m_tok == '#' || m_tok == Token::INT) { //file input
      int i1 = getFileNum();
      match(',');
      FInput *f1 = m_nodeMan.make<FInput>(i1);
      while (true) {
         f1->ids.push_back(getId());
         if (m_tok != ',')
            break;
         peek();
      }
      return f1;
   } else { //key input
      string prompt;

      if (m_tok == Token::STRING) {
         prompt = m_l.sval;
         peek();
         match(';');
      }
      Input *i = m_nodeMan.make<Input>(prompt);
      while (true) {
         i->ids.push_back(getId());
         if (m_tok != ',')
            break;
         peek();
      }
      return i;
   }
}

inline Stmt *Compiler::writestmt() {
   peek();
   int i1 = getFileNum();
   match(',');
   Write *w = m_nodeMan.make<Write>(i1);
   while (true) {
      w->exprs.push_back(expr());
      if (m_tok != ',')
         break;
      peek();
   }
   return w;
}

inline Stmt *Compiler::readstmt() {
   peek();
   Read *r = m_nodeMan.make<Read>();
   while (true) {
      r->ids.push_back(getId());
      if (m_tok != ',')
         break;
      peek();
   }
   return r;
}

inline Stmt *Compiler::openstmt() {
   peek();
   Expr *fn = expr(Value::Type::STRING);
   match(Token::FOR);

   File::Mode mode;
   switch (m_tok) {
   case Token::INPUT:
      mode = File::Mode::INPUT;
      peek();
      break;
   case Token::ID:
      if ("OUTPUT" == m_l.sval) {
         mode = File::Mode::OUTPUT;
         peek();
      } else if ("RANDOM" == m_l.sval) {
         mode = File::Mode::RANDOM;
         peek();
      } else if ("OUTPUTAS" == m_l.sval) {
         mode = File::Mode::OUTPUT;
         m_tok = Token::AS;
      } else if ("RANDOMAS" == m_l.sval) {
         mode = File::Mode::RANDOM;
         m_tok = Token::AS;
      } else if ("INPUTAS" == m_l.sval) {
         mode = File::Mode::INPUT;
         m_tok = Token::AS;
      } else if ("APPEND" == m_l.sval) {
         mode = File::Mode::APPEND;
         peek();
      } else if ("APPENDAS" == m_l.sval) {
         mode = File::Mode::APPEND;
         m_tok = Token::AS;
      } else if ("BINARY" == m_l.sval) {
         mode = File::Mode::BINARY;
         peek();
      } else {
         cerror("File mode error: [%s]", m_l.sval);
      }
      break;
   default:
      cerror("Not file mode: [%c]", m_tok);
   }
   match(Token::AS);

   int i2 = getFileNum();
   int len;
   if (mode == File::Mode::RANDOM && m_tok == Token::ID && m_l.sval == "LEN") {
      peek();
      match('=');
      len = m_l.ival;
      match(Token::INT);
   } else
      len = Open::NOLEN;
   return m_nodeMan.make<Open>(i2, fn, mode, len);
}

inline Stmt *Compiler::onstmt() {
   peek();
   Expr *e1 = expr(Value::Type::REAL);
   bool isSub;
   if (m_tok == Token::GOTO) {
      isSub = false;
   } else if (m_tok == Token::GOSUB) {
      isSub = true;
   } else {
      cerror("Token error: [%c], expecting GOTO / GOSUB", m_tok);
   }
   peek();

   On *o = m_nodeMan.make<On>(e1, isSub);
   On::Addr addr;

   while (true) {
      addr.label = m_l.ival;
      match(Token::INT);
      o->addrs.push_back(addr);
      if (m_tok != ',')
         break;
      peek();
   }

   return findLabel(o);
}

inline Stmt *Compiler::fieldstmt() {
   peek();
   int fnum = getFileNum();
   match(',');

   Field *f1 = m_nodeMan.make<Field>(fnum);
   int total = 0;
   while (true) {
      int size = m_l.ival;
      match(Token::INT);
      match(Token::AS);
      string id = m_l.sval;
      match(Token::ID);
      if (getIdType(id) != Value::Type::STRING) {
         cerror("Need string Id in FIELD: [%s]", id);
      }
      f1->fields.push_back(make_pair(size, id));
      total += size;
      if (m_tok != ',')
         break;
      peek();
   }

   f1->total = total;
   return f1;
}

inline int Compiler::getFileNum() {
   if (m_tok == '#')
      peek();
   int i = m_l.ival - 1;
   match(Token::INT);
   if (i > 2 || i < 0) {
      cerror("File number error: [%i]", i + 1);
   }
   return i;
}

inline Stmt *Compiler::findLabel(Goto *s) {
   auto i = m_labels.find(s->label);
   if (i != m_labels.end()) {
      s->stm = i->second;
   } else {
      m_refs.push_back(make_tuple(s, m_line, m_label));
   }
   return s;
}

inline Stmt *Compiler::findLabel(Restore *s) {
   if (Restore::NO_LABEL == s->label)
      return s;
   if (!m_labels.count(s->label)) {
      m_refs.push_back(make_tuple(s, m_line, m_label));
   }
   return s;
}

inline Stmt *Compiler::findLabel(On *s) {
   m_refs.push_back(make_tuple(s, m_line, m_label));
   return s;
}


enum Precedence {
   PREC_NONE = 0,
   PREC_LOG,
   PREC_REL,
   PREC_ADD,
   PREC_MUL,
   PREC_NEG,
   PREC_POW,
   PREC_NOT
};

// 参数必须是REAL或STRING
inline Expr *Compiler::expr(Value::Type vtype) {
   Expr *e = E_(PREC_NONE);
   if (getRValType(e->vtype) != vtype) {
      cerror("Rvalue type error: [%t], expecting [%t]", e->vtype, vtype);
   }
   return e;
}

inline Expr *Compiler::expr() {
   return E_(PREC_NONE);
}

Expr *Compiler::E_(int prev_p) {
   Expr *e1 = F();
   int cur_p;
   while ((cur_p = getp(m_tok)) > prev_p) {
      int i = m_tok;
      peek();
      Expr *e2 = E_(cur_p);

      if (getRValType(e1->vtype) != getRValType(e2->vtype)) {
         cerror("Incompatible operand type in binary: [%t, %t]",
                e1->vtype, e2->vtype);
      }
      switch (i) {
      case '-': case '*': case '/': case '^': case Token::AND: case Token::OR:
         if (getRValType(e1->vtype) != Value::Type::REAL) {
            cerror("Rvalue type error in binary: [%t], [%t] expected",
                   e1->vtype, Value::Type::REAL);
         }
      default:
         break;
      }
      e1 = m_nodeMan.make<Binary>(e1, e2, i, '+' == i ? e1->vtype
                                                     : Value::Type::REAL);
   }
   return e1;
}

Expr *Compiler::F() {
   switch (m_tok) {
   case Token::INT: {
      double r = m_l.ival; // 一定valid
      peek();
      return m_nodeMan.make<Real>(r);
   }

   case Token::REAL: {
      double r = m_l.rval;
      peek();

      auto res = RealHelper::validate(r);
      assert(res != RealHelper::Result::IS_NAN);
      if (RealHelper::Result::IS_INF == res) {
         cerror("Number overflow");
      }

      return m_nodeMan.make<Real>(r);
   }

   case Token::STRING: {
      string s = addCNPrefix(m_l.sval);
      peek();
      return m_nodeMan.make<Str>(s);
   }

   case '+': case '-': {
      int i1 = m_tok;
      peek();
      Expr *e1 = E_(PREC_NEG);
      if (Value::Type::REAL != getRValType(e1->vtype)) {
         cerror("Rvalue type error in unary: [%t], expecting [%t]",
                e1->vtype, Value::Type::REAL);
      }
      return '-' == i1 ?
             m_nodeMan.make<FuncCall>(Func::Type::NEG, Value::Type::REAL, e1)
            : e1;
   }

   case Token::NOT: {
      peek();
      Expr *e1 = E_(PREC_NOT);
      if (Value::Type::REAL != getRValType(e1->vtype)) {
         cerror("Rvalue type error in NOT: [%t], expecting [%t]",
                e1->vtype, Value::Type::REAL);
      }
      return m_nodeMan.make<FuncCall>(Func::Type::NOT, Value::Type::REAL, e1);
   }

   case Token::INKEY:
      peek();
      return m_nodeMan.make<Inkey>();

   case '(': {
      peek();
      Expr *e1 = expr();
      match(')');
      return e1;
   }

   case Token::FN: {
      peek();
      string id = m_l.sval;
      match(Token::ID);
      match('(');
      Expr *e1 = expr();
      match(')');
      return m_nodeMan.make<UserCall>(e1, id, getIdType(id));
   }

   case Token::ID:
      return idexpr();

   default:
      cerror("Token error: [%c], expression expected", m_tok);
   }
}

inline Expr *Compiler::idexpr() {
   auto it = s_builtinFuncs.find(m_l.sval);
   if (s_builtinFuncs.end() != it) { // 是系统函数
      Func::Type ft = it->second;
      peek();
      match('(');

      Expr *e1 = expr(Func::getParamType(ft, 0));
      Expr *e2 = nullptr, *e3 = nullptr;
      switch (ft) {
      case Func::Type::LEFT:
      case Func::Type::RIGHT:
      case Func::Type::POINT:
         match(',');
         e2 = expr(Value::Type::REAL);
         break;
      case Func::Type::MID:
         match(',');
         e2 = expr(Value::Type::REAL);
         if (',' == m_tok) {
            peek();
            e3 = expr(Value::Type::REAL);
         }
      default:
         break;
      }
      match(')');

      return m_nodeMan.make<FuncCall>(ft, Func::getReturnType(ft), e1, e2, e3);
   }

   // 是id
   Id *id1 = getId();
   // id1->vtype = getRValType(id1->vtype);
   return id1;
}

inline int Compiler::getp(int op) {
   switch (op) {
   case Token::GE: case Token::LE: case Token::NEQ:
   case '=': case '>': case '<':
      return PREC_REL;
   case '+': case '-':
      return PREC_ADD;
   case '*': case '/':
      return PREC_MUL;
   case '^':
      return PREC_POW;
   case Token::AND: case Token::OR:
      return PREC_LOG;
   default:
      return PREC_NONE;
   }
}

Value::Type Compiler::getIdType(const string &id) {
   switch (id.back()) {
   case '$': return Value::Type::STRING;
   case '%': return Value::Type::INT;
   default: return Value::Type::REAL;
   }
}

inline const char *Compiler::getTypeSuffix(const std::string &s) {
   switch (s.back()) {
   case '$': return "$";
   case '%': return "%";
   default:
      return "";
   }
}

Value::Type Compiler::getIdRValType(const std::string &id) {
   return getRValType(getIdType(id));
}

Value::Type Compiler::getRValType(Value::Type type) {
   return static_cast<Value::Type>(static_cast<int>(type) & Value::RVAL_MASK);
}

Id *Compiler::getId() {
    string id = m_l.sval;

   match(Token::ID);
   Value::Type vt = getIdType(id);
   if ('(' == m_tok) {
      peek();
      vector<Expr *> v;

      while (true) {
         v.push_back(expr(Value::Type::REAL));
         if (')' == m_tok) {
            peek();
            break;
         }
         match(',');
      }
      return m_nodeMan.make<ArrayAccess>(id, v, vt);
   } else {
      return m_nodeMan.make<Id>(id, vt);
   }
}

Stmt *Compiler::translate(Stmt *head, Stmt *end) {
   Stmt *cur = head;

   while (cur != end) {
      switch (cur->type) {
      default:
Ldefault:
         cur = cur->next;
         break;
      case Stmt::Type::IF: {
         If *i = static_cast<If *>(cur);
         i->stmThen = translate(i->stmThen, cur->next);
         if (i->stmElse)
            i->stmElse = translate(i->stmElse, cur->next);
         goto Ldefault;
      }
      case Stmt::Type::ASSIGN:
         /* for i=1 to n:next
          * for i=0 to n:next         ==>  sleep n
          * 10 ... : for i=1 to n
          * 20 next
          * */
         if (cur->next != end && cur->next->type == Stmt::Type::FOR) {
            Assign *a1 = static_cast<Assign *>(cur);

            if (a1->val->type == Expr::Type::REAL) {
               Real *r = static_cast<Real *>(a1->val);

               if (1.0 == r->rval || 0.0 == r->rval) {
                  For *f1 = static_cast<For *>(cur->next);

                  if (!f1->step) {
                     Stmt *t = f1;

                     while (t->next != end
                            && t->next->type == Stmt::Type::NEWLINE)
                        t = t->next;

                     if (t->next != end && t->next->type == Stmt::Type::NEXT) {
                        Next *n1 = static_cast<Next *>(t->next);
                        if (n1->var.empty() || n1->var == f1->var) {
                           // remove NEXT
                           t->next = n1->next;
                           m_nodeMan.destroy(n1);

                           m_nodeMan.destroy(r); // for i=X to n, remove X
                           a1->val = f1->dest; // for i=X to n  -->  i = n
                           XSleep *s1 = m_nodeMan.make<XSleep>(a1->val);
                           s1->next = f1->next;
                           a1->next = s1;
                           m_nodeMan.destroy(f1);

                           cur = s1->next;
                           break;
                        }
                     }
                  }
               }
            }
         }
         goto Ldefault;
      }
   }
   return head;
}

// 汉字加上0x1f前缀
inline string &Compiler::addCNPrefix(std::string &s) {
   for (size_t i = 0; i < s.size(); ) {
      if (s[i++] & 128) {
         if (i < s.size()) {
            s.insert(i - 1, 1, 0x1f);
            i += 2;
         }
      }
   }

   return s;
}

inline string &Compiler::rtrim(std::string &s) {
   int i = static_cast<int>(s.size());
   while (--i >= 0 && ' ' == s[i])
      ;
   s.resize(static_cast<string::size_type>(i + 1));
   return s;
}