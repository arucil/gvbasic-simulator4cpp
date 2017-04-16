#include "node_man.h"

using namespace gvbsim;

NodeManager::NodeManager() {
   head = new Node;
   head->node.prev = head->node.next = head;
}

NodeManager::~NodeManager() {
   clear();
   delete head;
}

void NodeManager::clear() {
   for (Node *p = head->node.next; p != head; ) {
      Node *t = p;
      p = p->node.next;
      delete t;
   }
   head->node.prev = head->node.next = head;
}

void NodeManager::destroy(Node *p) {
   p->node.prev->node.next = p->node.next;
   p->node.next->node.prev = p->node.prev;
   delete p;
}
