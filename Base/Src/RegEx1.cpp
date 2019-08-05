//
//  Copyright (c)1998-2012, Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: File.h 252 2013-12-18 04:40:28Z  $
//

#include "Base/RegEx.h"
#include "Base/BaseTemplate.h"
#include "regex/regex.h"

namespace Public {
namespace Base {

struct RegEx::RegExInternal
{
	regex_t compiled;
	bool compiledresult;
};
RegEx::RegEx(const std::string& pattern, RegExType type)
{
	internal = new RegExInternal;

	memset(&internal->compiled, 0, sizeof(internal->compiled));

	int cflags = REG_EXTENDED;
	if (type == RegExType_InCase)
		cflags |= REG_ICASE;

	internal->compiledresult = regcomp(&internal->compiled, pattern.c_str(), cflags) == 0;
	
}
RegEx::~RegEx()
{
	regfree(&internal->compiled);

	SAFE_DELETE(internal);
}
bool RegEx::regex_match(const std::string& str, const RegEx& regexstr)
{
	if (!regexstr.internal->compiledresult) return false;

	regmatch_t match = {0};

	return regexec(&regexstr.internal->compiled, str.c_str(), 1, &match, 0) == 0;
}

} // namespace Base
} // namespace Public


