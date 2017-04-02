#ifndef GVBASIC_NODE_MGR_H
#define GVBASIC_NODE_MGR_H

#include <type_traits>
#include "node.h"

namespace gvbsim {

class NodeManager {
   Node *head;

public:
   NodeManager();
   ~NodeManager();

public:
   template <typename T, typename... Ts>
   T *make(Ts... args);

   void destroy(Node *);

   void clear();
};

template <typename T, typename... Ts>
T *NodeManager::make(Ts... args) {
   static_assert(std::is_base_of<Node, T>::value);

   T *p = new T(args...);
   p->node.next = head;
   p->node.prev = head->node.prev;
   head->node.prev->node.next = p;
   head->node.prev = p;

   return p;
}

}

#endif //GVBASIC_MEM_MGR_H
