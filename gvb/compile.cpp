#include "compile.h"
#include <cassert>
#include "gvb.h"
#include "node_man.h"
#include "data_man.h"
#include "tree.h"

using namespace gvbsim;
using namespace std;

#define cerror(...)  GVB::error(m_line, m_label, __VA_ARGS__)

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
   for (auto t : m_refs) {
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
         linkStmt(getStmtTail(i->stm), next);
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
      }
      }
   }
}