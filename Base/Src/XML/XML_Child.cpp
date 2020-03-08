#include "Base/XML.h"
#include "tinyxml/tinyxml.h"
#include "XMLTinyxml.h"
namespace Public
{
namespace Base
{

struct XMLChildData
{
	std::list<XML::Attribute> attributeList;
	std::list<XML::Child> childList;
	std::string name;
	Value value;
};

struct XML::AttributeIterator::AttributeIteratorInternal
{
	shared_ptr<XMLChildData> data;

	std::list<XML::Attribute>::iterator iter;

	void set(const shared_ptr<XMLChildData> &_data)
	{
		data = _data;
		iter = data->attributeList.begin();
	}
};

XML::AttributeIterator::AttributeIterator()
{
	internal = new AttributeIteratorInternal();
}
XML::AttributeIterator::AttributeIterator(const AttributeIterator& iterator)
{
	internal = new AttributeIteratorInternal();
	internal->data = iterator.internal->data;
	internal->iter = iterator.internal->iter;
}
XML::AttributeIterator::~AttributeIterator()
{
	SAFE_DELETE(internal);
}
XML::AttributeIterator::operator bool()
{
	return internal->iter != internal->data->attributeList.end();
}
const XML::Attribute *XML::AttributeIterator::operator->()
{
    XML::AttributeIterator& attrinter = *this;

    const XML::Attribute& attr = *attrinter;

    
	return &attr;
}
const XML::Attribute &XML::AttributeIterator::operator*()
{
	static XML::Attribute empty;
	if (internal->iter == internal->data->attributeList.end())
		return empty;

	return *internal->iter;
}
XML::AttributeIterator &XML::AttributeIterator::operator++(int)
{
	internal->iter++;
	return *this;
}
XML::AttributeIterator & XML::AttributeIterator::operator=(const AttributeIterator &iterator)
{
    internal->data = iterator.internal->data;
    internal->iter = iterator.internal->iter;

    return *this;
}
static void parseKey(const std::string &key, std::string &type, std::string &name)
{
	name = key;
	size_t pos = String::indexOf(key, ":");
	if (pos != (size_t)-1)
	{
		type = std::string(key.c_str(), pos);
		name = std::string(key.c_str() + pos + 1);
	}
}
static bool isSameKey(const std::string &key1, const std::string &key2)
{
	std::string type1, name1, type2, name2;
	parseKey(key1, type1, name1);
	parseKey(key2, type2, name2);

	if (name1 != name2)
		return false;
	if (type1.length() > 0 && type2.length() > 0 && type1 != type2)
		return false;

	return true;
}

struct XML::ChildIterator::ChildIteratorInternal
{
	shared_ptr<XMLChildData> data;

	std::list<XML::Child>::iterator iter;
	std::string searchChildName;

	void set(const shared_ptr<XMLChildData> &_data)
	{
		data = _data;
		iter = data->childList.begin();
		findIter();
	}

	void findIter()
	{
		while (1)
		{
			if (iter == data->childList.end())
				break;

			if (searchChildName.length() <= 0 || isSameKey(iter->name(), searchChildName))
				break;
			iter++;
		}
	}
};

XML::ChildIterator::ChildIterator()
{
	internal = new ChildIteratorInternal();
}
XML::ChildIterator::ChildIterator(const ChildIterator& iterator)
{
	internal = new ChildIteratorInternal();
	internal->data = iterator.internal->data;
	internal->iter = iterator.internal->iter;
	internal->searchChildName = iterator.internal->searchChildName;
}
XML::ChildIterator::~ChildIterator()
{
	SAFE_DELETE(internal);
}
XML::ChildIterator::operator bool()
{
	return internal->iter != internal->data->childList.end();
}
const XML::Child *XML::ChildIterator::operator->()
{
	return &**this;
}
const XML::Child &XML::ChildIterator::operator*()
{
	static XML::Child empty;
	if (internal->iter == internal->data->childList.end())
		return empty;

	return *internal->iter;
}
XML::ChildIterator &XML::ChildIterator::operator++(int)
{
	internal->iter++;
	internal->findIter();

	return *this;
}
XML::ChildIterator & XML::ChildIterator::operator=(const ChildIterator &iterator)
{
    internal->data = iterator.internal->data;
    internal->iter = iterator.internal->iter;
    internal->searchChildName = iterator.internal->searchChildName;

    return *this;
}
struct XML::Child::ChildInternal
{
	ChildInternal() { }
	~ChildInternal() {}

	shared_ptr<XMLChildData> data;

	XML::Child &getChild(const std::string &key, int index)
	{
		int getIndex = 0;
		std::list<XML::Child>::iterator iter;
		for (iter = data->childList.begin(); iter != data->childList.end(); iter++)
		{
			if (key.empty() || isSameKey(iter->name(), key))
			{
				if (getIndex == index)
				{
					return *iter;
				}
				getIndex++;
			}
		}

		static Child emptyChild;

		return emptyChild;
	}

	Value &attribute(const std::string &key)
	{
		std::list<XML::Attribute>::iterator iter;
		for (iter = data->attributeList.begin(); iter != data->attributeList.end(); iter++)
		{
			if (key.empty() || isSameKey(iter->name(), key))
			{
				return iter->value();
			}
		}

		static Value emptyAttri;

		return emptyAttri;
	}
};
XML::Child::Child(const std::string &_name, const Value &_data)
{
	internal = new ChildInternal;
    internal->data = make_shared<XMLChildData>();
	name(_name);
	if (!_data.empty())
		data(_data);
}
XML::Child::Child(const Child &child)
{
	internal = new ChildInternal;
	internal->data = child.internal->data;
}
XML::Child::~Child()
{
	delete internal;
}

void XML::Child::name(const std::string &_name)
{
	internal->data->name = _name;
}
const std::string &XML::Child::name() const
{
	return internal->data->name;
}
void XML::Child::data(const Value &value)
{
	internal->data->value = value;
}
const Value &XML::Child::data() const
{
	return internal->data->value;
}
XML::Child::operator Value() const
{
	return data();
}
XML::Child &XML::Child::addChild(const std::string &key, const Value &val)
{
	return addChild(XML::Child(key, val));
}
XML::Child &XML::Child::addChild(const XML::Child &child)
{
	internal->data->childList.push_back(child);

	return internal->data->childList.back();
}
XML::Child &XML::Child::getChild(const std::string &name, int index)
{
	return internal->getChild(name, index);
}

const XML::Child &XML::Child::getChild(const std::string &name, int index) const
{
	return internal->getChild(name, index);
}

void XML::Child::removeChild(const std::string &key, int index)
{
	int getIndex = 0;
	for (std::list<XML::Child>::iterator iter = internal->data->childList.begin(); iter != internal->data->childList.end(); iter++)
	{
		if (isSameKey(iter->name(), key))
		{
			if (getIndex == index)
			{
				internal->data->childList.erase(iter);
				break;
			}
			getIndex++;
		}
	}
}
int XML::Child::childCount() const
{
	return (int)internal->data->childList.size();
}

int XML::Child::attributeCount() const
{
	return (int)internal->data->attributeList.size();
}
XML::Attribute &XML::Child::addAttribute(const std::string &key, const Value &val)
{
	return addAttribute(XML::Attribute(key, val));
}
XML::Attribute &XML::Child::addAttribute(const XML::Attribute &attri)
{
	for (std::list<XML::Attribute>::iterator iter = internal->data->attributeList.begin(); iter != internal->data->attributeList.end(); iter++)
	{
		if (iter->name() == attri.name())
		{
			*iter = attri;

			return *iter;
		}
	}

	internal->data->attributeList.push_back(attri);

	return internal->data->attributeList.back();
}
Value &XML::Child::getAttribute(const std::string &key)
{
	return internal->attribute(key);
}
const Value &XML::Child::getAttribute(const std::string &key) const
{
	return internal->attribute(key);
}

void XML::Child::removeAttribute(const std::string &key)
{
	std::list<XML::Attribute>::iterator iter;
	for (iter = internal->data->attributeList.begin(); iter != internal->data->attributeList.end();)
	{
		if (isSameKey(iter->name(), key))
		{
			internal->data->attributeList.erase(iter++);
		}
		else
		{
			iter++;
		}
	}
}

XML::ChildIterator XML::Child::child(const std::string &childname) const
{
	XML::ChildIterator iter;
	iter.internal->searchChildName = childname;
	iter.internal->set(internal->data);

	return iter;
}
XML::AttributeIterator XML::Child::attribute() const
{
	XML::AttributeIterator iter;
	iter.internal->set(internal->data);

	return iter;
}

bool XML::Child::isEmpty() const
{
	return internal->data->name.length() == 0 && (internal->data->attributeList.size() == 0 && internal->data->childList.size() == 0);
}

void XML::Child::clear()
{
	internal->data = make_shared<XMLChildData>();
}

XML::Child &XML::Child::operator=(const Child &child)
{
	internal->data = child.internal->data;

	return *this;
}
bool XML::Child::operator==(const Child &child) const
{
	return internal->data->name == child.internal->data->name &&
		   internal->data->value == child.internal->data->value &&
		   internal->data->attributeList == child.internal->data->attributeList &&
		   internal->data->childList == child.internal->data->childList;

	return true;
}
XML::Child::operator bool() const
{
	return !isEmpty();
}

} // namespace Base
} // namespace Public
