#include "compile.h"
#include <cassert>
#include "gvb.h"
#include "data_man.h"
#include "tree.h"

using namespace gvbsim;
using namespace std;

#define cerror(...)  GVB::error(m_line, m_label, __VA_ARGS__)


#define f(s)  { #s, Func::Type::s }
#define fs(s) { #s "$", Func::Type::s }

const unordered_map<string, Func::Type> Compiler::s_builtinFuncs = {
      f(ABS), f(ASC), f(ATN), f(COS), f(EXP), f(INT), f(LEN), f(LOF),
      f(LOG), f(PEEK), f(POS), f(RND), f(SGN), f(SIN), f(SQR), f(TAN), f(VAL),
      fs(CHR), fs(CVI), fs(CVS), fs(LEFT), fs(MID), fs(MKI), fs(MKS), fs(RIGHT),
      fs(STR), { "EOF", Func::Type::FEOF }
};

#undef fs
#undef f


Compiler::Compiler(FILE *fp, NodeManager &nm, DataManager &dm)
      : m_l(fp), m_nodeMan(nm), m_dataMan(dm) {
}

void Compiler::peek() {
   try {
      m_tok = m_l.getToken();
   } catch (int) {
      cerror("Number format error");
   }
}

void Compiler::match(int t) {
   if (t != m_tok)
      cerror("Token error: [%m_c], expecting [%m_c]", m_tok, t);
   peek();
}

Stmt *Compiler::compile() {
   m_nodeMan.clear();
   m_dataMan.clear();
   m_line = 1;

   peek();

   Stmt *prev;
   NewLine *head = nullptr;
   NewLine *stmts;

   while (m_tok != -1) {
      stmts = aLine(); //不可能为null
      if (m_tok != 10 && m_tok != -1)
         cerror("Token error: [%m_c], expecting EOL / EOF", m_tok);
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

   return head;
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
   m_label = m_l.ival;
   match(Token::INT);
   if (m_labels.count(m_label))
      cerror("Label duplicate");
   m_dataMan.addLabel(m_label);

   NewLine *s = m_nodeMan.make<NewLine>(m_line, m_label);
   s->next = stmts();
   return m_labels[m_label] = s; //记录一行的第一条语句
}

inline Stmt *Compiler::stmts() { //可能为null
   Stmt *head = nullptr, *cur;

   while (true) {
      Stmt *stm = stmt(false);//可能为null
      if (!stm)
         break;
      if (!head) {
         head = cur = stm;
      } else {
         linkStmt(cur, stm);
      }
      cur = getStmtTail(cur);
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
         int t = m_tok;
         peek();
         int i1 = m_l.ival;
         match(Token::INT);
         return findLabel(m_nodeMan.make<Goto>(i1, Token::GOSUB == t));
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
      if (!stm)
         break;
      if (!head) {
         head = cur = stm;
      } else {
         linkStmt(cur, stm);
      }
      cur = getStmtTail(cur);
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
            m_tok = i1 == 13 ? 10 : i1;
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
         m_tok = i1 == 13 ? 10 : i1;
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

   return m_nodeMan.make<DefFn>(f, var, expr(getIdRValType(f)));
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
      p1->exprs.push_back(make_pair<Expr *>(nullptr, Print::Delimiter::NO));
   }
   return p1;
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

inline Expr *Compiler::expr(Value::Type vtype) {
   Expr *e = E_(PREC_NONE);
   if (e->vtype != vtype) {
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

      if (e1->vtype != e2->vtype) {
         cerror("Incompatible operand type in binary: [%t, %t]",
                e1->vtype, e2->vtype);
      }
      switch (i) {
      case '-': case '*': case '/': case '^': case Token::AND: case Token::OR:
         if (e1->vtype != Value::Type::REAL) {
            cerror("Rvalue type error in binary: [%t], [%t] expected",
                   e1->vtype, Value::Type::REAL);
         }
      }
      e1 = m_nodeMan.make<Binary>(e1, e2, i,'+' == i ? e1->vtype
                                                     : Value::Type::REAL);
   }
   return e1;
}

Expr *Compiler::F() {
   switch (m_tok) {
   case Token::INT: {
      double r = m_l.ival;
      peek();
      return m_nodeMan.make<Real>(r);
   }
   case Token::REAL: {
      double r = m_l.rval;
      peek();
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
      if (Value::Type::REAL != e1->vtype) {
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
      if (Value::Type::REAL != e1->vtype) {
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
      return m_nodeMan.make<UserCall>(e1, id, getIdRValType(id));
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
   id1->vtype = getRValType(id1->vtype);
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

inline Value::Type Compiler::getIdType(const string &id) {
   switch (id.back()) {
   case '$': return Value::Type::STRING;
   case '%': return Value::Type::INT;
   default: return Value::Type::REAL;
   }
}

inline Value::Type Compiler::getIdRValType(const std::string &id) {
   return getRValType(getIdType(id));
}

inline Value::Type Compiler::getRValType(Value::Type type) {
   return static_cast<Value::Type>(static_cast<int>(type)
         & static_cast<int>(Value::Type::RVAL_MASK));
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

// 汉字加上0x1f前缀
inline string &Compiler::addCNPrefix(std::string &s) {
   int count = 0;
   for (size_t i = 0; i < s.size(); ++i) {
      if (s[i] & 128)
         ++count, ++i;
   }
   if (!count)
      return s;

   s.resize(s.size() + count);
   for (int i = static_cast<int>(s.size()), j = i + count; --i >= 0; ) {
      if (s[i] & 128) {
         s[--j] = s[i];
         s[--j] = s[--i];
         s[--j] = 31;
      } else
         s[--j] = s[i];
   }

   return s;
}

inline string &Compiler::rtrim(std::string &s) {
   int i = static_cast<int>(s.size());
   while (--i >= 0 && ' ' == s[i])
      ;
   s.resize(i + 1);
   return s;
}