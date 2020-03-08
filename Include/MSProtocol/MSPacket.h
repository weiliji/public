#pragma once 
#include "Defs.h"
#include "Base/Base.h"
using namespace Public::Base;


namespace Milesight {
namespace Protocol {

struct FiledDesInfo;
struct MSPacket
{
public:	
	virtual const char* getTypeName()const = 0;
//	std::vector<shared_ptr<FiledDesInfo>>	unknown_fields;
public:
	//从内存中解析到对象
	static bool MSProtocol_Export parseFromString(const std::string& str, MSPacket& pkg);
	//从json字符串中解析到对象
	static bool MSProtocol_Export parseFromJson(const std::string& str, MSPacket& pkg);
	//将对象序列化为字符串
	static std::string MSProtocol_Export serializeAsString(const MSPacket& pkg);
	//将对象序列化为json
	static std::string MSProtocol_Export serializeAsJson(const MSPacket& pkg);
};


}
}