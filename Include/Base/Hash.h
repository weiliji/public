//
//  Copyright (c)1998-2012, Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: Hash.h 252 2013-12-18 04:40:28Z  $
//

#pragma once

#include "Base/Defs.h"
#include "Base/IntTypes.h"

namespace Public {
namespace Base {

class BASE_API Hash
{
public:
	//使用 bkdr_hash 进行hash计算,输出uint64值
	static uint64_t bkdr(const std::string& data);
};

}
}
