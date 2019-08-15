//
//  Copyright (c)1998-2014, Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: Function.h 3 2013-01-21 06:57:38Z  $
//
#pragma  once
#include "Base/Shared_ptr.h"
#include "Base/BaseTemplate.h"
namespace Public {
namespace Base {

/// \class Function
/// \brief 函数指针类模块
///
/// 支持普通函数指针和成员函数指针的保存，比较，调用等操作。对于成员函数，
/// 要求类只能是普通类或者是单继承的类，不能是多继承或虚继承的类。
/// 根据参数个数会被替换成Function，用户通过Function<R, T1, T2, T3,..,TN>方式来使用，
/// R表示返回值类型，TN表示参数类型， N表示函数参数个数，目前最大参数为6。示例如下：
/// \code
/// int g(int x)
/// {
/// 	return x * 2;
/// }
/// class G
/// {
/// public:
/// 	int g(int x)
/// 	{
/// 		return x * 3;
/// 	}
/// }gg;
/// void test()
/// {
/// 	Function<int, int> f1(g);
/// 	Function<int(int)> f1(g);
/// 	Function<int, int> f2(&G::g, &gg);
/// 	assert(f1(1) = 2);
/// 	assert(f2(1) = 3);
/// 	f1 = f2;
/// 	assert(f1 == f2);
/// }
/// \endcode


////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////函数定义///////////////////////////////////////////

template <typename R, typename... ARGS>
class _FunctionImpl
{
public:

	typedef R(*PTR_FUNCTION)(ARGS ...);

	struct IDENT
	{
		void* func;
		void* obj;
		bool operator==(const IDENT& other) const
		{
			if (func == other.func && obj == other.obj)
			{
				return true;
			}
			return false;
		}
		bool operator<(const IDENT& other) const
		{
			if (func < other.func) return true;
			if (func == other.func && obj < other.obj) return true;
			return false;
		}
	};
	//function，基础对象
	struct FUNCTION_OBJECT
	{
		FUNCTION_OBJECT() {}
		virtual ~FUNCTION_OBJECT() {}

		virtual bool empty() const = 0;

		virtual R invoke(ARGS ... a) = 0;
		virtual IDENT ident() = 0;
	};

	//函数指针的定义
	struct FUNCTION_POINTER :public FUNCTION_OBJECT
	{
		FUNCTION_POINTER(PTR_FUNCTION _f) :f(_f) {}

		virtual bool empty() const { return f == NULL; }

		virtual R invoke(ARGS ... a)
		{
			if(f == NULL) return INIT_VALUE<R>::Value();

			return f(a ...);
		}
		virtual IDENT ident()
		{
			IDENT identi;
			identi.func = (void*)f;
			identi.obj = NULL;
			return identi;
		}

		PTR_FUNCTION f;
	};

	//成员函数回调的定义
	template<typename O>
	struct FUNCTION_MEMBER :public FUNCTION_OBJECT
	{
		typedef R(O::*MEM_FUNCTION)(ARGS ...);

		FUNCTION_MEMBER(MEM_FUNCTION _f, const O * _o):f(_f),o((O *)_o){}
		FUNCTION_MEMBER(MEM_FUNCTION _f, const shared_ptr<O>& _sptr1) :f(_f), o(NULL), wptr(shared_ptr<O>()), sptr(_sptr1) {}
		FUNCTION_MEMBER(MEM_FUNCTION _f, const weak_ptr<O>& _wptr) :f(_f), o(NULL),wptr(_wptr),sptr(NULL){}		


		virtual bool empty() const { return f == NULL; }

		virtual R invoke(ARGS ... a)
		{
			O* optr = o;
			shared_ptr<O> ptrtmp = wptr.lock();

			if (ptrtmp == NULL) ptrtmp = sptr;
			if (ptrtmp != NULL) optr = ptrtmp.get();

			if (optr == NULL || f == NULL)
			{
				return INIT_VALUE<R>::Value();
			}

			return (optr->*f)(a ...);
		}
		virtual IDENT ident()
		{
			IDENT identi;
			identi.func = &f;
			identi.obj = o;
			shared_ptr<O> tmp = wptr.lock();
			if (tmp == NULL) tmp = sptr;
			if (tmp != NULL)
			{
				identi.obj = tmp.get();
			}
			return identi;
		}

		MEM_FUNCTION	f;
		O*				o;
		weak_ptr<O>		wptr;
		shared_ptr<O>   sptr;
	};

	shared_ptr<FUNCTION_OBJECT> _impl;
protected:
	/// 缺省构造函数
	_FunctionImpl( ){}
	~_FunctionImpl() {}
public:
	/// 将类的成员函数和类对象的指针绑定并保存。其他类型的函数指针可以=运算符和隐式转换直接完成。
	template<typename O>
	void bind(R(O::*f)(ARGS ...), const O * o)
	{
		realse();

		if(f == NULL || o == NULL)
		{
			return;
		}

		_impl = shared_ptr<FUNCTION_MEMBER<O> >(new FUNCTION_MEMBER<O>(f,o));
	}

	void bind(R (*f)(ARGS ...))
	{
		realse();

		if (f == NULL)
		{
			return;
		}
		_impl = shared_ptr<FUNCTION_POINTER>(new FUNCTION_POINTER(f));
	}
	template<typename O>
	void bind(R(O::*f)(ARGS ...), const shared_ptr<O>& ptr)
	{
		realse();

		if(f == NULL || ptr == NULL)
		{
			return;
		}
		_impl = shared_ptr<FUNCTION_MEMBER<O> >(new FUNCTION_MEMBER<O>(f, ptr));
	}
	template<typename O>
	void bind(R(O::*f)(ARGS ...), const weak_ptr<O>& wptr)
	{
		realse();

		if (f == NULL || wptr.lock() == NULL)
		{
			return;
		}
		_impl = shared_ptr<FUNCTION_MEMBER<O> >(new FUNCTION_MEMBER<O>(f, wptr));
	}
	operator bool() const
	{
		shared_ptr<FUNCTION_OBJECT> tmp = _impl;
		if (tmp == NULL) return false;

		return !tmp->empty();
	}

	bool operator!() const
	{
		return !(bool(*this));
	}	

	/// 重载()运算符，可以以函数对象的形式来调用保存的函数指针。
	inline R operator()(ARGS ... a)
	{
		shared_ptr<FUNCTION_OBJECT> tmp = _impl;

		if (tmp == NULL)
		{
			return INIT_VALUE<R>::Value();
		}

		return tmp->invoke(a ...);
	}
	
	void realse()
	{
		_impl = NULL;
	}
};

template <typename R, typename... ARGS>
class Function:public _FunctionImpl<R,ARGS ...>
{
public:
	Function() {}

	/// 接受成员函数指针构造函数，将类的成员函数和类对象的指针绑定并保存。
	/// \param [in] f 类的成员函数
	/// \param [in] o 类对象的指针
	template<typename O>
	Function(R(O::*f)(ARGS ...), const O * o)
	{
		bind(f, o);
	}

	template<typename O>
	Function(R(O::*f)(ARGS ...), const shared_ptr<O>& ptr)
	{
		bind(f, ptr);
	}

	template<typename O>
	Function(R(O::*f)(ARGS ...), const weak_ptr<O>& ptr)
	{
		bind(f, ptr);
	}

	/// 接受普通函数指针构造函数，保存普通函数指针。
	/// \param [in] f 函数指针
	Function(R(*f)(ARGS ...))
	{
		bind(f);
	}
	Function(const Function& f)
	{
		swap(f);
	}

	~Function()
	{
		realse();
	}
	/// 拷贝构造函数
	/// \param [in] f 源函数指针对象
	Function& operator=(const Function& f)
	{
		swap(f);

		return *this;
	}

	bool operator<(const Function& tpl) const
	{
		shared_ptr<FUNCTION_OBJECT> tmp = _impl;
		shared_ptr<FUNCTION_OBJECT> tmp1 = tpl._impl;
		if (tmp == NULL || tmp1 == NULL)
		{
			return false;
		}
		return tmp->ident() < tmp1->ident();
	}
	void swap(const Function& f)
	{
		_impl = f._impl;
	}
};

template <typename R, typename... ARGS>
class Function<R(ARGS...)> :public _FunctionImpl<R, ARGS ...>
{
public:
	Function() {}
	
	/// 接受成员函数指针构造函数，将类的成员函数和类对象的指针绑定并保存。
	/// \param [in] f 类的成员函数
	/// \param [in] o 类对象的指针
	template<typename O>
	Function(R(O::*f)(ARGS ...), const O * o)
	{
		bind(f, o);
	}

	template<typename O>
	Function(R(O::*f)(ARGS ...), const shared_ptr<O>& ptr)
	{
		bind(f, ptr);
	}

	template<typename O>
	Function(R(O::*f)(ARGS ...), const weak_ptr<O>& ptr)
	{
		bind(f, ptr);
	}

	/// 接受普通函数指针构造函数，保存普通函数指针。
	/// \param [in] f 函数指针
	Function(R(*f)(ARGS ...))
	{
		bind(f);
	}
	Function(const Function& f)
	{
		swap(f);
	}

	~Function()
	{
		realse();
	}
	/// 拷贝构造函数
	/// \param [in] f 源函数指针对象
	Function& operator=(const Function& f)
	{
		swap(f);

		return *this;
	}

	bool operator<(const Function& tpl) const
	{
		shared_ptr<FUNCTION_OBJECT> tmp = _impl;
		shared_ptr<FUNCTION_OBJECT> tmp1 = tpl._impl;
		if (tmp == NULL || tmp1 == NULL)
		{
			return false;
		}
		return tmp->ident() < tmp1->ident();
	}
	void swap(const Function& f)
	{
		_impl = f._impl;
	}
};

}
}
