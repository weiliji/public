#include "Base/Base.h"
#include "SQLiteRecordset.h"
#include "sqlite3.h"
using namespace Public::Base;

struct SQLiteRecordset::SQLiteRecordsetInternal
{
	CharSetEncoding charset;
	sqlite3_stmt *stmt3;

	SQLiteRecordsetInternal() : stmt3(NULL) {}
	~SQLiteRecordsetInternal()
	{
		if (stmt3 != NULL)
		{
			sqlite3_finalize(stmt3);
		}
	}
};

SQLiteRecordset::SQLiteRecordset()
{
	internal = new SQLiteRecordsetInternal();
}
SQLiteRecordset::~SQLiteRecordset()
{
	SAFE_DELETE(internal);
}

shared_ptr<SqlDB_Row> SQLiteRecordset::getRow()
{
	if (internal->stmt3 == NULL)
		return shared_ptr<SqlDB_Row>();

	if (sqlite3_step(internal->stmt3) != SQLITE_ROW)
	{
		return shared_ptr<SqlDB_Row>();
	}

	shared_ptr<SqlDB_Row> set = make_shared<SqlDB_Row>();

	int clouns = sqlite3_column_count(internal->stmt3);

	for (int i = 0; i < clouns; i++)
	{
		int ret = sqlite3_column_type(internal->stmt3, i);

		switch (ret)
		{
		case SQLITE_INTEGER:
		{
			uint64_t filedval = sqlite3_column_int64(internal->stmt3, i);
			set->putFiled(i, filedval);
		}
		break;
		case SQLITE_TEXT:
		{
			const unsigned char *filedval = sqlite3_column_text(internal->stmt3, i);

			if (filedval != NULL)
			{
				if (internal->charset == CharSetEncoding_UTF8)
				{
					set->putFiled(i, String::utf82ansi((char *)filedval));
				}
				else
				{
					set->putFiled(i, (char *)filedval);
				}
			}
		}
		break;
		case SQLITE_BLOB:
		{
			const void *bolbdata = sqlite3_column_blob(internal->stmt3, i);
			int bolblen = sqlite3_column_bytes(internal->stmt3, i);

			set->putFiled(i, Value(std::string((const char *)bolbdata, bolblen)));
		}
		break;
		case SQLITE_FLOAT:
		{
			double filedval = sqlite3_column_double(internal->stmt3, i);
			set->putFiled(i, filedval);
		}
		break;
		default:
			break;
		}
	}

	return set;
}

struct SQLiteConnection::SQLiteConnectionInternal
{
	CharSetEncoding charset;
	sqlite3 *sqlhand;
	Mutex	mutex;

	bool beginTransaction()
	{
		return exec("begin transaction");
	}
	bool commitTransaction()
	{
		return exec("commit transaction");
	}
	bool exec(const std::string &sql)
	{
		char *errmsg;

		int ret = sqlite3_exec(sqlhand, sql.c_str(), NULL, NULL, &errmsg);
		if (ret != 0)
		{
			logerror("sqlite3_exec:%s error:%s\r\n", sql.c_str(), errmsg);
		}

		sqlite3_free(errmsg);

		return ret == 0;
	}
	bool execBlob(const std::string &sql, const std::vector<std::string> &bloblval)
	{
		sqlite3_stmt *stmt = NULL;
		int ret = sqlite3_prepare(sqlhand, sql.c_str(), (int)sql.length(), &stmt, 0);
		if (ret != 0)
		{
			logerror("execBlob sqlite3_prepare:%s error\r\n", sql.c_str());

			return false;
		}

		for (size_t i = 0; i < bloblval.size(); i++)
		{
			const std::string &string = bloblval[i];

			ret = sqlite3_bind_blob(stmt, (int)(i + 1), string.c_str(), (int)string.length(), NULL);
			if (ret != 0)
			{
				const char *errmsg = sqlite3_errmsg(sqlhand);

				logerror("sqlite3_bind_blob:%s index %d error:%s\r\n", string.c_str(), i + 1, errmsg);

				return false;
			}
		}
		ret = sqlite3_step(stmt);
		if (ret != 0 && ret != SQLITE_DONE)
		{
			const char *errmsg = sqlite3_errmsg(sqlhand);

			logerror("sqlite3_step:%s error:%s\r\n", sql.c_str(), errmsg);

			return false;
		}
		ret = sqlite3_finalize(stmt);
		if (ret != 0)
		{
			const char *errmsg = sqlite3_errmsg(sqlhand);

			logerror("sqlite3_finalize:%s error:%s\r\n", sql.c_str(), errmsg);

			return false;
		}

		return true;
	}
	static bool backup(sqlite3 *tosql, sqlite3 *fromsql)
	{
		if (tosql == NULL || fromsql == NULL)
			return false;

		int rc = SQLITE_OK;
		sqlite3_backup *pBackup = sqlite3_backup_init(tosql, "main", fromsql, "main");
		if (pBackup == NULL)
		{
			return false;
		}

		sqlite3_backup_step(pBackup, -1);

		(void)sqlite3_backup_finish(pBackup);

		rc = sqlite3_errcode(tosql);

		return rc == SQLITE_OK;
	}

	std::string dbname;
	bool create;
};

SQLiteConnection::SQLiteConnection(const std::string &dbname, bool create, CharSetEncoding charset)
{
	internal = new SQLiteConnection::SQLiteConnectionInternal;

	internal->charset = charset;
	internal->sqlhand = NULL;
	internal->dbname = dbname;
	internal->create = create;
}

SQLiteConnection::~SQLiteConnection()
{
	disconnect();

	delete internal;
}

bool SQLiteConnection::connect()
{
	Guard locker(internal->mutex);

	if (!internal->create && !File::access(internal->dbname.c_str(), File::accessExist))
	{
		return false;
	}
	if (internal->sqlhand != NULL)
	{
		return false;
	}
	int ret = sqlite3_open_v2(String::ansi2utf8(internal->dbname).c_str(), &internal->sqlhand, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL);

	if (ret != 0)
	{
		std::string errstring = sqlite3_errmsg(internal->sqlhand);
		internal->sqlhand = NULL;
		logerror("sqlite3_open %s error %s\r\n", internal->dbname.c_str(), errstring.c_str());
	}
	return ret == 0;
}

bool SQLiteConnection::integrityCheck()
{
	Guard locker(internal->mutex);
	if (internal->sqlhand == NULL)
		return false;

	// integrity_check检查包括：乱序的记录、缺页、错误的记录、丢失的索引、唯一性约束、非空约束
	//if ( sqlite3_prepare_v2( obj_db, "PRAGMA integrity_check;", -1, &integrity, NULL ) == SQLITE_OK )
	//quick_check不检查约束条件，耗时较短

	shared_ptr<SqlDB_Recordset> set = query("PRAGMA integrity_check;");
	if (set == NULL)
		return false;

	shared_ptr<SqlDB_Row> row = set->getRow();
	if (row == NULL)
		return false;

	std::string result = row->getField();

	return String::iequals(result, "ok");
}

bool SQLiteConnection::disconnect()
{
	Guard locker(internal->mutex);
	if (internal->sqlhand)
	{
		sqlite3_close(internal->sqlhand);
		internal->sqlhand = NULL;
	}

	return true;
}

bool SQLiteConnection::exec(const SqlDB_Sql &sql, uint32_t *affectedrow)
{
	Guard locker(internal->mutex);
	if (internal->sqlhand == NULL)
	{
		return false;
	}

	std::string sqlstr = sql.str(internal->charset);
	std::vector<std::string> bloblval = sql.getBolbValue();

	if (bloblval.size() <= 0)
		return internal->exec(sqlstr);
	else
		return internal->execBlob(sqlstr, bloblval);
}

bool SQLiteConnection::exec(const std::vector<SqlDB_Sql> &sql, uint32_t *affectedrow)
{
	Guard locker(internal->mutex);
	if (internal->sqlhand == NULL)
	{
		return false;
	}

	if (!internal->beginTransaction())
	{
		return false;
	}
	bool ret = true;
	for (unsigned int i = 0; i < sql.size(); i++)
	{
		std::string sqlstr = sql[i].str(internal->charset);
		std::vector<std::string> bloblval = sql[i].getBolbValue();

		if (bloblval.size() <= 0)
			ret = internal->exec(sqlstr);
		else
			ret= internal->execBlob(sqlstr, bloblval);

		if (!ret)
			break;
	}

	internal->commitTransaction();

	return ret;
}

CharSetEncoding SQLiteConnection::charset() const
{
	return internal->charset;
}

shared_ptr<SqlDB_Recordset> SQLiteConnection::query(const SqlDB_Sql &sql)
{
	Guard locker(internal->mutex);

	shared_ptr<SQLiteRecordset> set = make_shared<SQLiteRecordset>();

	std::string sqlstr = sql.str(internal->charset);

	if (internal->sqlhand == NULL)
	{
		return set;
	}

	set->internal->charset = internal->charset;

	if (sqlite3_prepare_v2(internal->sqlhand, /*ensql*/ sqlstr.c_str(), -1, &set->internal->stmt3, NULL) != 0)
	{
		//return shared_ptr<SqlDB_Recordset>();
	}

	return set;
}
uint32_t SQLiteConnection::count(const SqlDB_Sql &sql)
{
	if (internal->sqlhand == NULL)
	{
		return 0;
	}

	shared_ptr<SqlDB_Recordset> set = query(sql);

	if (set == NULL)
	{
		return 0;
	}

	shared_ptr<SqlDB_Row> rowset = set->getRow();
	if (rowset != NULL)
	{
		return 0;
	}

	return rowset->getField(0).readUint32();
}

bool SQLiteConnection::backup(const std::string &tofile)
{
	Guard locker(internal->mutex);
	if (internal->sqlhand == NULL)
		return false;

	sqlite3 *tosql = NULL;

	int rc = sqlite3_open(tofile.c_str(), &tosql);
	if (rc != SQLITE_OK)
		return false;

	bool ret = SQLiteConnectionInternal::backup(tosql, internal->sqlhand);

	sqlite3_close(tosql);

	return ret;
}
bool SQLiteConnection::recovery(const std::string &fromfile)
{
	Guard locker(internal->mutex);
	if (!File::access(fromfile, File::accessExist))
	{
		return false;
	}

	if (internal->sqlhand == NULL)
		return false;

	sqlite3 *tosql = NULL;

	int rc = sqlite3_open(fromfile.c_str(), &tosql);
	if (rc != SQLITE_OK)
		return false;

	bool ret = SQLiteConnectionInternal::backup(internal->sqlhand, tosql);

	sqlite3_close(tosql);

	return ret;
}