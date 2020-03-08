#ifndef _LIMITCPU_H__
#define _LIMITCPU_H__
#include "Base/Defs.h"

namespace Public
{
namespace Base
{
class BASE_API CPU
{
	CPU();

public:
	~CPU();

	// 获取cpu核心数
	// @param logiccpu 逻辑处理器
	// @param corecpu 核心处理器
	static bool getCpuCount(unsigned int &logiccpu, unsigned int &corecpu);

	// 按比例限制cpu运行
	// scale 比例，比如限制 4/1的cpu
	static bool limit(unsigned int scale);
};
} // namespace Base
} // namespace Public

#endif