#pragma  once
#include "Base/IntTypes.h"
#include "Base/Defs.h"
#include "Base/Value.h"
#include "Base/ErrorInfo.h"

namespace Public{
namespace Base {

class BASE_API JsonValue
{
public:
	/** \brief Type of the value held by a Value object.
	*/
	enum ValueType
	{
		nullValue = 0, ///< 'null' value
		intValue,      ///< signed integer value
		realValue,     ///< double value
		stringValue,   ///< UTF-8 string value
		booleanValue,  ///< bool value
		arrayValue,    ///< array value (ordered list)
		objectValue    ///< object value (collection of name/value pairs).
	};

	enum WriteStyle
	{
		WriteStyle_Normal = 0,
		WriteStyle_Fast, //无回车格式
	};
	enum CommentPlacement
	{
		commentBefore = 0,        ///< a comment placed on the line before a value
		commentAfterOnSameLine,   ///< a comment just after a value on the same line
		commentAfter,             ///< a comment on the line after a value (only make sense for root value)
		numberOfCommentPlacement
	};
public:
	JsonValue(ValueType type = nullValue);
	JsonValue(const JsonValue& other);

	//初始化一个值的json
	JsonValue(const Value& val);
	//初始化一个数组json
	JsonValue(const std::vector<JsonValue>& val);
	//初始化一个对象json
	JsonValue(const std::map<std::string, JsonValue>& val);

	~JsonValue();

	JsonValue& operator=(const JsonValue& other);

	ValueType type() const;
	operator ValueType() const;

	bool operator <(const JsonValue& other) const;
	bool operator <=(const JsonValue& other) const;
	bool operator >=(const JsonValue& other) const;
	bool operator >(const JsonValue& other) const;

	bool operator ==(const JsonValue& other) const;
	bool operator !=(const JsonValue& other) const;

	/// Return isNull()
	bool operator!() const;

	// < return -1, > return 1,== return 0
	int compare(const JsonValue& other) const;

	bool isNull() const;
	bool isBool() const;
	bool isInt() const;
	bool isIntegral() const;
	bool isDouble() const;
	bool isNumeric() const;
	bool isString() const;
	bool isArray() const;
	bool isObject() const;

	//以下接口为value不为array和object时有效
	Value* operator->();
	const Value* operator->()const;

	Value& operator*();
	const Value& operator*()const;

	operator Value();

	Value& data();
	const Value& data() const;


	//以下接口为数组专用

	/// Number of values in array or object
	size_t size() const;

	/// \brief Return true if empty array, empty object, or null;
	/// otherwise, false.
	bool empty() const;

	/// Remove all object members and array elements.
	/// \pre type() is arrayValue, objectValue, or nullValue
	/// \post type() is unchanged
	void clear();

	/// Resize the array to size elements. 
	/// New elements are initialized to null.
	/// May only be called on nullValue or arrayValue.
	/// \pre type() is arrayValue or nullValue
	/// \post type() is arrayValue
	void resize(size_t size);

	/// Access an array element (zero based index ).
	/// If the array contains less than index element, then null value are inserted
	/// in the array so that its size is index+1.
	/// (You may need to say 'value[0u]' to get your compiler to distinguish
	///  this from the operator[] which takes a string.)

	JsonValue& operator[](size_t index);
	/// Access an array element (zero based index )
	/// (You may need to say 'value[0u]' to get your compiler to distinguish
	///  this from the operator[] which takes a string.)

	const JsonValue& operator[](size_t index) const;

	/// If the array contains at least index+1 elements, returns the element value, 
	/// otherwise returns defaultValue.
	const JsonValue& get(size_t index, const JsonValue& defaultValue) const;

	/// Return true if 0 <= index < size().
	bool isValidIndex(size_t index) const;

	/// \brief Append value to array at the end.
	///
	/// Equivalent to jsonvalue[jsonvalue.size()] = value;
	JsonValue& append(const JsonValue& value);

	std::vector<JsonValue>& array();
	const std::vector<JsonValue>& array()const;


	//以下接口专为对象使用
	/// Access an object value by name, create a null member if it does not exist.
	JsonValue& operator[](const std::string& key);
	JsonValue& operator[](const char* key);
	/// Access an object value by name, returns null if there is no member with that name.
	const JsonValue& operator[](const std::string& key) const;
	const JsonValue& operator[](const char* key) const;

	/// Return the member named key if it exist, defaultValue otherwise.
	JsonValue get(const std::string& key, const JsonValue& defaultValue) const;

	/// Same as removeMember(const char*)
	JsonValue removeMember(const std::string& key);

	/// Return true if the object has a member named key.
	bool isMember(const std::string& key) const;

	std::map<std::string, JsonValue>& object();
	const  std::map<std::string, JsonValue>& object()const;

	//一下为注释
	/// Comments must be //... or /* ... */
	void setComment(const std::string &comment,CommentPlacement placement);
	bool hasComment(CommentPlacement placement) const;
	/// Include delimiters and embedded newlines.
	std::string getComment(CommentPlacement placement) const;
	const std::map<CommentPlacement, std::string> comments() const;

	//以下为序列化反序列化相关
	std::string write(WriteStyle style = WriteStyle_Normal) const;
	std::string toStyledString() const;
	ErrorInfo readFile(const std::string& filename);
	ErrorInfo read(const std::string& stream);
	ErrorInfo read(const char* bufferaddr, int buferlen);
private:
	struct JsonValueInternal;
	JsonValueInternal* internal;
};

}
} // namespace Json

