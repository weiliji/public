#include "Base/XML.h"
#include "Base/PrintLog.h"
#include "tinyxml/tinyxml.h"
#include "XMLTinyxml.h"
namespace Public{
namespace Base{

struct XMLAttributeData
{
	std::string name;
	Value		value;
};

struct XML::Attribute::AttributeInternal
{
	shared_ptr<XMLAttributeData> data;
};

XML::Attribute::Attribute(const std::string& name, const Value& value)
{
	internal = new AttributeInternal();
    internal->data = make_shared<XMLAttributeData>();
	internal->data->name = name;
	internal->data->value = value;
}

XML::Attribute::Attribute(const Attribute& attri)
{
	internal = new AttributeInternal();
	internal->data = attri.internal->data;
}
XML::Attribute::~Attribute()
{
	SAFE_DELETE(internal);
}
bool XML::Attribute::isEmpty() const
{
	return internal->data->name == "" || internal->data->value.empty();
}

bool XML::Attribute::operator == (const Attribute& val)const
{
	return internal->data->name == val.internal->data->name && internal->data->value == val.internal->data->value;
}
XML::Attribute & XML::Attribute::operator=(const Attribute &val)
{
    internal->data = val.internal->data;

    return *this;
}
XML::Attribute::operator bool() const
{
	return !isEmpty();
}
std::string& XML::Attribute::name()
{
	return internal->data->name;
}
const std::string& XML::Attribute::name() const
{
	return internal->data->name;
}
Value& XML::Attribute::value()
{
	return internal->data->value;
}
const Value& XML::Attribute::value() const
{
	return internal->data->value;
}

}
}
