#include "Base/SqlDataBase.h"
#include "Base/Value.h"
#include "Base/BaseTemplate.h"
namespace Public {
namespace Base {

struct SqlDB_Row::SqlDB_RowInternal
{
	std::map<int,Value>		FiledIndexList;
	uint32_t				getIndex = 0;
};

SqlDB_Row::SqlDB_Row()
{
	internal = new SqlDB_RowInternal();
}
SqlDB_Row::~SqlDB_Row()
{
	SAFE_DELETE(internal);
}
int SqlDB_Row::getColumnCount()
{
	return (int)internal->FiledIndexList.size();
}
void SqlDB_Row::putFiled(int index, const Value& val)
{
	internal->FiledIndexList[index] = val;
}
const Value SqlDB_Row::getField(int index) const
{
	static Value tmp;
	std::map<int, Value>::const_iterator rIter = internal->FiledIndexList.find(index);
	if (rIter != internal->FiledIndexList.end())
	{
		return rIter->second;
	}

	return tmp;
}

const Value SqlDB_Row::firstFiled() const 
{
	Value val = getField(0);

	if (!val.empty()) internal->getIndex = 1;

	return val;
}
const Value SqlDB_Row::nextFiled() const 
{
	Value val = getField(internal->getIndex);

	internal->getIndex++;

	return val;
}


struct SQLValue
{
	bool		isBlob = false;
	Value		value;
};


struct SQLDBData
{
	std::string sqltmp;

	std::vector<SQLValue> valuetmp;
};

struct SqlDB_Sql::SqlDB_SqlInternal
{
	shared_ptr< SQLDBData> data;

	static string converSqlString(const std::string& sql)
	{
		string sqltmp;
		const char* tmp = sql.c_str();
		size_t len = sql.length();

		for (size_t i = 0; i < len; i++)
		{
			if ((tmp[i] & 0xff) == '\'')
			{
				sqltmp.push_back('\'');
			}
			sqltmp.push_back(tmp[i]);
		}

		return sqltmp;
	}
};
SqlDB_Sql::SqlDB_Sql()
{
	internal = new SqlDB_SqlInternal;
	internal->data = make_shared<SQLDBData>();
}
SqlDB_Sql::SqlDB_Sql(const std::string& sqltmp)
{
	internal = new SqlDB_SqlInternal;
	internal->data = make_shared<SQLDBData>();
	internal->data->sqltmp = sqltmp;
}
SqlDB_Sql::SqlDB_Sql(const char* sqltmp)
{
	internal = new SqlDB_SqlInternal;
	internal->data = make_shared<SQLDBData>();
    if (sqltmp)
    {
        internal->data->sqltmp = sqltmp;
    }
}
SqlDB_Sql::SqlDB_Sql(const SqlDB_Sql& sql)
{
	internal = new SqlDB_SqlInternal;
	internal->data = sql.internal->data;

}
SqlDB_Sql::~SqlDB_Sql()
{
	SAFE_DELETE(internal);
}

SqlDB_Sql& SqlDB_Sql::operator << (const Value& val)
{
	SQLValue value;
	value.isBlob = false;
	value.value = val;

	internal->data->valuetmp.push_back(value);

	return *this;
}
SqlDB_Sql& SqlDB_Sql::operator << (const SQLBlobValue& val)
{
	SQLValue value;
	value.isBlob = true;
	value.value = val.value;

	internal->data->valuetmp.push_back(value);

	return *this;
}
SqlDB_Sql& SqlDB_Sql::operator = (const SqlDB_Sql& sql)
{
	internal->data = sql.internal->data;

    return *this;
}
std::string SqlDB_Sql::str(CharSetEncoding charset) const
{
	std::string newstr;
	uint32_t index = 0;

	const char* sqltmpstr = internal->data->sqltmp.c_str();
	while (*sqltmpstr)
	{
		if (*sqltmpstr != '?')
		{
			newstr.push_back(*sqltmpstr);
		}
		else if(index < internal->data->valuetmp.size())
		{
			const SQLValue& tmpval = internal->data->valuetmp[index];

			if (tmpval.isBlob)
			{
				newstr.push_back(*sqltmpstr);
			}
			else
			{
				const Value& tmp = tmpval.value;

				if (tmp.empty() || tmp.type() == Value::Type_String)
				{
					newstr += "'";
				}

				if (tmp.type() == Value::Type_Bool) newstr += tmp.readBool() ? '1' : '0';
				else
				{
					if (charset == CharSetEncoding_UTF8)
					{
						newstr += SqlDB_SqlInternal::converSqlString(String::ansi2utf8(tmp.readString()));
					}
					else
					{
						newstr += SqlDB_SqlInternal::converSqlString(tmp.readString());
					}
				}
				if (tmp.empty() || tmp.type() == Value::Type_String)
				{
					newstr += "'";
				}
			}
			index++;
		}
		sqltmpstr++;
	}

	return newstr;
}

std::vector<std::string> SqlDB_Sql::getBolbValue() const
{
	std::vector<std::string> bloblval;
	for (size_t i = 0; i < internal->data->valuetmp.size(); i++)
	{
		if (internal->data->valuetmp[i].isBlob)
		{
			bloblval.push_back(internal->data->valuetmp[i].value);
		}
	}

	return bloblval;
}

} // namespace Base
} // namespace Public

