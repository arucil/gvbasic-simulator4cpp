#include <cassert>
#include <cmath>
#include "../real.h"

using namespace std;
using namespace gvbsim;

#define PI (atan(1) * 4)

namespace {

#define CHECK_FROM_DOUBLE(d, l)  assert(Real::fromDouble(d) == (l));

void testFromDouble() {
   CHECK_FROM_DOUBLE(0.0, 0x00'0000'0000L);
   CHECK_FROM_DOUBLE(-0.0, 0x00'8000'0000L);
   CHECK_FROM_DOUBLE(1.0, 0x80'0000'0000L);
   CHECK_FROM_DOUBLE(-1.0, 0x80'8000'0000L);
   CHECK_FROM_DOUBLE(.5, 0x7F'0000'0000L);
   CHECK_FROM_DOUBLE(-.5, 0x7F'8000'0000L);
   CHECK_FROM_DOUBLE(10., 0x83'2000'0000L);
   CHECK_FROM_DOUBLE(-10., 0x83'A000'0000L);
   CHECK_FROM_DOUBLE(.25, 0x7E'0000'0000L);
}

#define CHECK_VALIDATE(d, l)   \
	do { \
		double d_ = d; \
		Real::Result r = Real::validate(d_); \
		assert(r == Real::Result::IS_VALID); \
		assert(Real::fromDouble(d_) == l); \
	} while (0);

void checkInfOrNaN(double d, Real::Result result) {
   double d1 = d;
   Real::Result r = Real::validate(d1);
   assert(r != Real::Result::IS_VALID);
   assert(r == result);
   assert(r == Real::Result::IS_NAN && isnan(d1)
          || r == Real::Result::IS_INF && isinf(d1)
             && (d > 0 && d1 > 0 || d < 0 && d1 < 0));
}

void testValidate() {
   CHECK_VALIDATE(0.0, 0x00'0000'0000L);
   CHECK_VALIDATE(-0.0, 0x00'8000'0000L);
   CHECK_VALIDATE(1.0, 0x80'0000'0000L);
   CHECK_VALIDATE(-1.0, 0x80'8000'0000L);
   CHECK_VALIDATE(.5, 0x7F'0000'0000L);
   CHECK_VALIDATE(-.5, 0x7F'8000'0000L);
   CHECK_VALIDATE(10., 0x83'2000'0000L);
   CHECK_VALIDATE(-10., 0x83'A000'0000L);
   CHECK_VALIDATE(.25, 0x7E'0000'0000L);
   CHECK_VALIDATE(sqrt(.5), 0x7f'3504f334L);
   CHECK_VALIDATE(sqrt(2), 0x80'3504f334L);
   CHECK_VALIDATE(log(2), 0x7f'317217f8L);
   CHECK_VALIDATE(log2(exp(1)), 0x80'38aa3b29L);
   CHECK_VALIDATE(PI / 2, 0x80'490fdaa2L);
   CHECK_VALIDATE(PI * 2, 0x82'490fdaa2L);
   CHECK_VALIDATE(1e38, 0xfe'16769951L);
   CHECK_VALIDATE(-1e38, 0xfe'96769951L);

   checkInfOrNaN(1e307, Real::Result::IS_INF);
   checkInfOrNaN(-1e307, Real::Result::IS_INF);
   checkInfOrNaN(1e39, Real::Result::IS_INF);
   checkInfOrNaN(-1e39, Real::Result::IS_INF);
   checkInfOrNaN(NAN, Real::Result::IS_NAN);
   checkInfOrNaN(INFINITY, Real::Result::IS_INF);
   checkInfOrNaN(-INFINITY, Real::Result::IS_INF);
}

#define CHECK_TO_DOUBLE(d, l)  assert(abs((d) - Real::toDouble(l)) < 1e-9)

void testToDouble() {
   CHECK_TO_DOUBLE(0.0, 0x00'0000'0000L);
   CHECK_TO_DOUBLE(-0.0, 0x00'8000'0000L);
   CHECK_TO_DOUBLE(1.0, 0x80'0000'0000L);
   CHECK_TO_DOUBLE(-1.0, 0x80'8000'0000L);
   CHECK_TO_DOUBLE(.5, 0x7F'0000'0000L);
   CHECK_TO_DOUBLE(-.5, 0x7F'8000'0000L);
   CHECK_TO_DOUBLE(10., 0x83'2000'0000L);
   CHECK_TO_DOUBLE(-10., 0x83'A000'0000L);
   CHECK_TO_DOUBLE(.25, 0x7E'0000'0000L);
   CHECK_TO_DOUBLE(sqrt(.5), 0x7f'3504f334L);
   CHECK_TO_DOUBLE(sqrt(2), 0x80'3504f334L);
   CHECK_TO_DOUBLE(log(2), 0x7f'317217f8L);
   CHECK_TO_DOUBLE(log2(exp(1)), 0x80'38aa3b29L);
   CHECK_TO_DOUBLE(PI / 2, 0x80'490fdaa2L);
   CHECK_TO_DOUBLE(PI * 2, 0x82'490fdaa2L);
}

}

#if 1

int main() {
   testFromDouble();
   testValidate();
   testToDouble();
}

#endif
