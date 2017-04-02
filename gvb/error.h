#ifndef GVBASIC_ERROR_H
#define GVBASIC_ERROR_H

#include <string>

namespace gvbsim {

struct Exception {
   int line, label;
   std::string msg;

public:
   Exception(int line, int label, const std::string &s)
         : line(line), label(label), msg(s) { }
};

}

#endif //GVBASIC_ERROR_H
