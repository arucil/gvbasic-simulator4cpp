#ifndef GVBASIC_DATA_MAN_H
#define GVBASIC_DATA_MAN_H

#include <unordered_map>
#include <vector>
#include <string>

namespace gvbsim {

class DataManager {
   std::unordered_map<int, size_t> labels; //label -> index
   std::vector<std::string> data;
   size_t p; // current index
public:
   DataManager() : p(0) {}

public:
   void restore();
   void restore(int label);
   void add(const std::string &);
   void addLabel(int);
   const std::string &get();
   size_t size() const;
   bool reachesEnd() const;
   void clear();
};

}

#endif //GVBASIC_DATA_MAN_H
