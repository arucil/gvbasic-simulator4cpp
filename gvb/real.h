#ifndef GVBASIC_REAL_H
#define GVBASIC_REAL_H

#include <cstdint>

namespace gvbsim {

/* gvb的浮点数是5字节的MBF(Microsoft Binary Format)格式，不支持inf、NaN和非规格化数。
 * gvb的两个函数CVS/MKS$能够将浮点数和二进制表示形式相互转换，为了能够支持这两个函数，
 * 而设计了这个类。
 * 浮点数在模拟器内部仍用double表示，仅在纠正实数运算结果和调用CVS/MKS$时使用这个类。
 *
 * 这里使用的5字节浮点数格式和MBF有些不同，暂且以MBF命名。
 *
 * 二进制格式：
 * highest  [ exp(8) sign(1)  mantissa(31) ]  lowest
 * bias=128, exponent ~ [-127, +127], -128 represents 0 (ignoring mantissa)
 */
class RealHelper {
public:
	enum class Result {
		IS_VALID,
		IS_NAN,
		IS_INF,
	};

public:
	RealHelper() = delete;

public:
   // double转换为MBF(只使用了uint64_t的最低5个字节)
   // 参数必须是经过validate()的合法double
	static uint64_t fromDouble(const double); // arg must be valid MBF

   // MBF转double
	static double toDouble(const uint64_t);

   // 纠正double
	static Result validate(double &);
};

}

#endif //GVBASIC_REAL_H
