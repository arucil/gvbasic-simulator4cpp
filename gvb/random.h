#ifndef GVBASIC_RANDOM_H
#define GVBASIC_RANDOM_H

#include <cstdint>

namespace gvbsim {

class Random {
public:
   enum { MAX = 65535 };

public:
   void setSeed(std::uint32_t seed) {
      this->seed = seed & MAX;
   }

   std::uint32_t random() {
      return seed = seed * 2053 + 13849 & MAX;
   }

   std::uint32_t sequence() {
      return seed = seed + 13849 & MAX;
   }

private:
   std::uint32_t seed;
};

}

#endif //GVBASIC_RANDOM_H
