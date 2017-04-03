#include "real.h"
#include <cassert>

using namespace gvbsim;
using namespace std;

uint64_t RealHelper::fromDouble(const double d) {
   uint64_t l = reinterpret_cast<const uint64_t &>(d);
   uint32_t sign = static_cast<uint32_t>(l >> 63);
   int32_t d_exp = l >> 52 & 0x7ff;

   if (!d_exp) { // 0
      return sign << 31;
   }

   // assert normal value
   assert(d_exp >= 1023 - 127 && d_exp <= 1023 + 127);
   // assert validated
   assert(0 == (l & 1 << 21 - 1));

   return sign << 31
          | static_cast<uint64_t>(d_exp - 1023 + 128) << 32
          | (l >> 52 - 31) & ((1u << 31) - 1);
}

double RealHelper::toDouble(const uint64_t l) {
   uint64_t d = (l & 0x8000'0000) << 32;
   int32_t exp = l >> 32 & 255;

   if (!exp) { // 0, ignore frac
      return reinterpret_cast<double &>(d);
   }

   d |= static_cast<uint64_t>(exp - 128 + 1023) << 52
        | l << 52 - 31 & 0xf'ffff'ffff'ffff;
   return reinterpret_cast<double &>(d);
}

RealHelper::Result RealHelper::validate(double &d) {
   uint64_t l = reinterpret_cast<uint64_t &>(d);
   int32_t exp = l >> 52 & 0x7ff;

   if (exp < 1023 - 127) { // 0 / denormal
      l &= 0x8000'0000'0000'0000L;
      d = reinterpret_cast<double &>(l);
      return Result::IS_VALID;
   }

   if (0x7ff == exp) { // inf / NaN
      return l & 0xf'ffff'ffff'ffffL ? Result::IS_NAN : Result::IS_INF;
   }

   if (exp > 1023 + 127) {
      // inf
Linf:
      l = l & 0x8000'0000'0000'0000L | 0x7ff0'0000'0000'0000L;
      d = reinterpret_cast<double &>(l);
      return Result::IS_INF;
   }

   // rounding
   uint32_t frac = (l >> 21 & 0x7fff'ffff) + ((l & 1 << 20) != 0);
   if (frac & 0x8000'0000) {
      if (++exp > 1023 + 127)
         goto Linf;
      frac &= 0x7fff'ffff;
   }

   l = l & 0x8000'0000'0000'0000L
       | static_cast<uint64_t>(exp) << 52
       | static_cast<uint64_t>(frac) << 21;
   d = reinterpret_cast<double &>(l);

   return Result::IS_VALID;
}
