//
//  Copyright (c)1998-2012, Public Technology
//  All Rights Reserved.
//
//
//	Description:
//	$Id: AtomicCount.h 80 2013-04-11 07:05:56Z  $
//

#ifndef __BASE_ATOMIC_COUNT_H__
#define __BASE_ATOMIC_COUNT_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#include <Windows.h>
#endif
#include "Base/Defs.h"

namespace Public {
namespace Base {
	
/// \class AtomicCount
/// \brief 原子计数类


class BASE_API AtomicCount
{
		/// 禁止拷贝构造函数和赋值操作
    AtomicCount( AtomicCount const & );
    AtomicCount & operator=( AtomicCount const & );
public:
	typedef int	ValueType;
public:
	AtomicCount();
	/// 原子计算构造函数
	/// \param v [in] 初始化时原子计数的值
	explicit AtomicCount( ValueType initValue);
	~AtomicCount();

	/// 前值递增操作函数
    ValueType operator++();

	/// 后值递增操作函数
	ValueType operator++(int);

	/// 前值递减去操作函数
    ValueType operator--();

	/// 后值递减去操作函数
	ValueType operator--(int);

	ValueType value() const;

	/// 重载函数调用操作
    operator ValueType() const;

	///判断原子计数非0
	bool operator !() const;
private:
#ifdef WIN32
	typedef volatile LONG  countType;
#else
	typedef int countType;
#endif
	countType		count;
};

/// \class AtomicCount
/// \brief 原子计数类

inline AtomicCount::AtomicCount() :count(0) {}

inline AtomicCount::AtomicCount(ValueType v) : count(v) {}

inline AtomicCount::~AtomicCount() {}

inline AtomicCount::ValueType AtomicCount::value() const
{
	return count;
}

inline AtomicCount::operator AtomicCount::ValueType() const
{
	return count;
}

inline bool AtomicCount::operator !() const
{
	return count == 0;
}

#ifdef WIN32
inline AtomicCount::ValueType AtomicCount::operator ++()
{
	return InterlockedIncrement(&count);
}
inline AtomicCount::ValueType AtomicCount::operator ++(int)
{
	AtomicCount::ValueType result = InterlockedIncrement(&count);

	return --result;
}
inline AtomicCount::ValueType AtomicCount::operator --()
{
	return InterlockedDecrement(&count);
}
inline AtomicCount::ValueType AtomicCount::operator --(int)
{
	AtomicCount::ValueType result = InterlockedDecrement(&count);

	return ++result;
}
#else
inline AtomicCount::ValueType AtomicCount::operator ++()
{
	return __sync_add_and_fetch(&count, 1);
}
inline AtomicCount::ValueType AtomicCount::operator ++(int)
{
	return __sync_fetch_and_add(&count, 1);
}
inline AtomicCount::ValueType AtomicCount::operator --()
{
	return __sync_sub_and_fetch(&count, 1);
}
inline AtomicCount::ValueType AtomicCount::operator --(int)
{
	return __sync_fetch_and_sub(&count, 1);
}
#endif

} // namespace Base
} // namespace Public

#endif // __BASE_ATOMIC_COUNT_H__


