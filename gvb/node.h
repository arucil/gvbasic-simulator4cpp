#ifndef GVBASIC_NODE_H
#define GVBASIC_NODE_H

namespace gvbsim {

struct Node {
   friend class NodeManager;

private:
   struct {
      Node *prev, *next; // 用于统一管理内存分配、释放
   } node;

protected:
   Node() = default;
};

}

#endif //GVBASIC_NODE_H
