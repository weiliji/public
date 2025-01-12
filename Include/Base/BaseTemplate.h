//
//  Copyright (c)1998-2012, Public Technology
//  All Rights Reserved.
//
//  Description:透传回应数据包.
//  $Id: BaseTemplate.h 237 2013-12-22 09:21:41Z  $
//

#ifndef _BASE_TEMPLATE_H_
#define _BASE_TEMPLATE_H_

namespace Public {
namespace Base {

template<class T> void SAFE_RELEASE( T &t )
{
	if ( t != NULL )
	{
		t->Release () ;
		t = NULL ;
	}
}

template<class T> void SAFE_DELETE( T &t )
{
	if (t!= NULL)
	{
		delete t ;
		t = NULL ;
	}
}

template<class T> void SAFE_DELETEARRAY( T &t )
{
	if (t!= NULL)
	{
		delete [] t ;
		t = NULL ;
	}
}

template<class T> void Init_Array( T *t, int count, T value )
{
	for ( int i = 0 ; i < count ; i++ )
	{
		t[i] = value ;
	}
}



//默认返回值的特例化处理
template<class R>
struct INIT_VALUE
{
	static R Value()
	{
		R* ptr = new(std::nothrow) R();
		R tmp(*ptr);
		delete ptr;
		return tmp;
	}
};

template<>
struct INIT_VALUE<void>
{
	static void Value()
	{
		return void();
	}
};


} // namespace Base
} // namespace Public

#endif	//_BASE_TEMPLATE_H_
