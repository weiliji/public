#ifndef __DBRECORDSETSQLITE_H__
#define __DBRECORDSETSQLITE_H__
#include "Base/Base.h"

namespace Public{
namespace Base{

class SQLiteRecordset : public SqlDB_Recordset
{
	friend class SQLiteConnection;
	struct SQLiteRecordsetInternal;

public:
	SQLiteRecordset();
	virtual ~SQLiteRecordset();
	shared_ptr<SqlDB_Row> getRow();

private:
	SQLiteRecordsetInternal *internal;
};

} // namespace Sqlite
} // namespace Public

#endif //__DBRECORDSET_H__
