
#ifndef __XML_H__
#define __XML_H__
#include "Base/Value.h"
#include "Base/Shared_ptr.h"
#include "Base/Defs.h"
#include "Base/String.h"
namespace Public
{
namespace Base
{

using namespace Public::Base;

class BASE_API XML
{
	struct XMLInternal;

public:
	class Child;
	struct BASE_API Attribute
	{
		Attribute(const std::string &name = "", const Value &value = Value());
		Attribute(const Attribute &attri);
		~Attribute();

		bool isEmpty() const;
		bool operator==(const Attribute &val) const;
        Attribute & operator=(const Attribute &val);
		operator bool() const;

		std::string &name();
		const std::string &name() const;
		Value &value();
		const Value &value() const;

	private:
		struct AttributeInternal;
		AttributeInternal *internal;
	};
	class BASE_API AttributeIterator
	{
		friend class Child;

	public:
		AttributeIterator();
		AttributeIterator(const AttributeIterator& iterator);
		~AttributeIterator();
		operator bool();
		const Attribute *operator->();
		const Attribute &operator*();
		AttributeIterator &operator++(int);
        AttributeIterator & operator=(const AttributeIterator &val);
	private:
		struct AttributeIteratorInternal;
		AttributeIteratorInternal *internal;
	};
	class BASE_API ChildIterator
	{
		friend class Child;

	public:
		ChildIterator();
		ChildIterator(const ChildIterator& iterator);
		~ChildIterator();
		operator bool();
		const Child *operator->();
		const Child &operator*();
		ChildIterator &operator++(int);
        ChildIterator & operator=(const ChildIterator &val);
	private:
		struct ChildIteratorInternal;
		ChildIteratorInternal *internal;
	};
	class BASE_API Child
	{
		friend class XML;
		struct ChildInternal;

	public:
		Child(const std::string &name = "", const Value &data = Value());
		Child(const Child &child);
		~Child();

		void name(const std::string &name);
		const std::string &name() const;

		void data(const Value &value);
		const Value &data() const;
		operator Value() const;

		Child &addChild(const std::string &key, const Value &val = Value());
		Child &addChild(const XML::Child &child);
		Child &getChild(const std::string &name, int index = 0);
		const Child &getChild(const std::string &name, int index = 0) const;
		void removeChild(const std::string &name, int index = 0);

		int childCount() const;
		int attributeCount() const;

		XML::Attribute &addAttribute(const std::string &key, const Value &val);
		XML::Attribute &addAttribute(const XML::Attribute &attri);
		Value &getAttribute(const std::string &key);
		const Value &getAttribute(const std::string &key) const;
		void removeAttribute(const std::string &key);

		ChildIterator child(const std::string &childname = "") const;
		AttributeIterator attribute() const;

		bool isEmpty() const;
		void clear();

		Child &operator=(const Child &child);
		bool operator==(const Child &child) const;

		operator bool() const;

	private:
		ChildInternal *internal;
	};

public:
	XML();
    XML(const XML& xml);
	~XML();

	bool parseFile(const std::string &file);
	bool parseBuffer(const std::string &buf);

	Child &body();
	const Child &body() const;

	std::string toString(CharSetEncoding encode = CharSetEncoding_Unknown) const;

	bool saveAs(const std::string &file, CharSetEncoding encode = CharSetEncoding_Unknown) const;

	bool isEmpty() const;
	void clear();

private:
	XMLInternal *internal;
};

} // namespace Base
} // namespace Public

#endif //__XML_H__
