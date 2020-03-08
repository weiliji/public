#include "Base/String.h"
using namespace Public::Base;

static Value getValueFromArgs(const std::vector<Value> &values, size_t &getindex)
{
	size_t getindextmp = getindex++;

	if (getindextmp >= values.size())
		return Value();

	return values[getindextmp];
}

int portable_vsnprintf1(char *str, size_t str_m, const char *fmt, const std::vector<Value> &values)
{
	size_t str_l = 0;
	const char *p = fmt;

	size_t getIndex = 1;

	if (!p)
		p = "";

	bool fmting = false;
	std::string fmtstr;

	while (*p && str_l < str_m && str)
	{
		//正在格式化中
		if (fmting)
		{
			//%aa%
			if (*p == '%')
			{
				if (fmtstr.length() == 1)
				{
					snprintf(str + str_l, str_m - str_l, "%%");
					str_l += strlen(str + str_l);
					fmting = false;
				}
				else
				{
					//当前这个情况从当前开始计算
					fmting = false;

					continue;
				}
			}
			else
			{
				fmtstr += *p;

				switch (*p)
				{
				case 'd':
				{
					snprintf(str + str_l, str_m - str_l, fmtstr.c_str(), getValueFromArgs(values, getIndex).readInt());
					str_l += strlen(str + str_l);
					fmting = false;

					break;
				}
				case 'u':
				{
					snprintf(str + str_l, str_m - str_l, fmtstr.c_str(), getValueFromArgs(values, getIndex).readUint64());
					str_l += strlen(str + str_l);
					fmting = false;

					break;
				}
				case 'f':
				case 'e':
				case 'g':
				{
					snprintf(str + str_l, str_m - str_l, fmtstr.c_str(), getValueFromArgs(values, getIndex).readFloat());
					str_l += strlen(str + str_l);
					fmting = false;

					break;
				}
				case 's':
				{
					snprintf(str + str_l, str_m - str_l, fmtstr.c_str(), getValueFromArgs(values, getIndex).readString().c_str());
					str_l += strlen(str + str_l);
					fmting = false;

					break;
				}
				case 'c':
				{
					snprintf(str + str_l, str_m - str_l, fmtstr.c_str(), (char)getValueFromArgs(values, getIndex).readInt());
					str_l += strlen(str + str_l);
					fmting = false;

					break;
				}
				case 'p':
				{
					snprintf(str + str_l, str_m - str_l, fmtstr.c_str(), getValueFromArgs(values, getIndex).readPointer());
					str_l += strlen(str + str_l);
					fmting = false;

					break;
				}
				case 'x':
				case 'X':
				case 'o':
				{
					snprintf(str + str_l, str_m - str_l, fmtstr.c_str(), getValueFromArgs(values, getIndex).readUint64());
					str_l += strlen(str + str_l);
					fmting = false;

					break;
				}
				default:
					break;
				}
			}
		}
		else if (*p == '%')
		{
			fmting = true;
			fmtstr = *p;
		}
		else
		{
			str[str_l] = *p;
			str_l++;
		}
		p++;
	}

	if (str_m > 0)
	{
		str[str_l <= str_m - 1 ? str_l : str_m - 1] = '\0';
	}
	return (int)str_l;
}