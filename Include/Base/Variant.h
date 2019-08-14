//
//  Copyright (c)1998-2012, Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: Math.h 3 2013-01-21 06:57:38Z  $
//

#ifndef __BASE_VARIANT_MATH_H__
#define __BASE_VARIANT_MATH_H__

#include "Base/IntTypes.h"
#include "Base/Shared_ptr.h"
#include "Base/Function.h"
#include "Defs.h"

namespace Public {
namespace Base {

//可变类型的类聚类
class Variant
{
	class variantobj
	{
	public:
		variantobj():valptr(NULL){}
		virtual ~variantobj() {}
		
		virtual shared_ptr<variantobj> clone() = 0;
		virtual std::string type() const = 0;

		template<typename T>
		T& get() const
		{
			if (typeid(T).name() != type())
			{
				static T val;
				return val;// FUNCTION_RETURN<T>::getDefaultValue();
			}
			return *(T*)valptr;
		}
	protected:
		void*	valptr;
	};

	template<typename T>
	class variantValObj:public variantobj
	{
	public:
		variantValObj() {}
		variantValObj(const T& val) 
		{
			valptr = new T(val);
		}
		~variantValObj()
		{
			if (valptr != NULL) delete (T*)valptr;
			valptr = NULL;
		}
		virtual shared_ptr<variantobj> clone()
		{
			shared_ptr<variantValObj> val = make_shared<variantValObj>();

			val->valptr = new T(*(T*)valptr);

			return val;
		}
		virtual std::string type()const { return typeid(T).name(); }
	};
public:
	Variant() {}
	template<typename T>
	Variant(const T& val)
	{
		value = shared_ptr<variantobj>(new variantValObj<T>(val));
	}
	~Variant() {}

	template<typename T>
	Variant operator=(const T& val)
	{
		value = shared_ptr<variantobj>(new variantValObj<T>(val));

		return *this;
	}

	template<typename T>
	T& get() const
	{
		if (value == NULL)
		{
			static T val;
			return val;// FUNCTION_RETURN<T>::getDefaultValue();
		}
		return value->get<T>();
	}

	std::string type() const 
	{
		if (value == NULL) return "none";

		return value->type();
	}

	bool empty() const { return value == NULL; }
private:
	shared_ptr<variantobj> value;
};


} // namespace Base
} // namespace Public

#endif// __BASE_MATH_H__

