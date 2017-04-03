#ifndef GVBASIC_DEBUG_H
#define GVBASIC_DEBUG_H

#include <ostream>

namespace gvbsim {

class Stmt;

void printTree(Stmt *, std::ostream &);

}

#endif //GVBASIC_DEBUG_H
