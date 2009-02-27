#include "ODBC.h"

NAMESPACE_UPP

class ODBCConnection : public SqlConnection
{
public:
	ODBCConnection(ODBCSession *session);
	virtual ~ODBCConnection();

	void                    Clear();

	virtual void            SetParam(int i, const Value& r);
	virtual bool            Execute();
	virtual int             GetRowsProcessed() const;
	virtual bool            Fetch();
	virtual void            GetColumn(int i, Ref r) const;
	virtual void            Cancel();
	virtual SqlSession&     GetSession() const { ASSERT(session); return *session; }
	virtual String          GetUser() const    { ASSERT(session); return session->user; }
	virtual String          ToString() const;
	virtual Value           GetInsertedId() const;

private:
	friend class ODBCSession;

	ODBCSession           *session;
	struct Param {
		int    ctype;
		int    sqltype;
		String data;
		SQLLEN li;
	};
	Array<Param>             param, bparam;
	String                   last_insert_table;

	int                      rowsprocessed;
	Vector< Vector<double> > number;
	Vector< Vector<String> > text;
	Vector< Vector<Time> >   time;
	int                      rowcount;
	int                      rowi;
	Vector<Value>            fetchrow;
	
	bool                   IsOk(SQLRETURN ret) const;
	void                   Flush();
	bool                   Fetch0();
	bool                   IsCurrent() const   { return session->current == this; }
};

bool ODBCSession::Connect(const char *cs)
{
	if(henv && IsOk(SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc))) {
		if(IsOk(SQLDriverConnect(hdbc, NULL, (SQLCHAR *)cs, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT))) {
			SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
			return true;
		}
		SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
		hdbc = SQL_NULL_HANDLE;
		SQLSetConnectAttr(hdbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, SQL_NTS);
		SQLSetConnectAttr(hdbc, SQL_ATTR_TXN_ISOLATION, (SQLPOINTER)SQL_TRANSACTION_SERIALIZABLE, SQL_NTS);
	}
	return false;
}

bool ODBCSession::IsOpen() const
{
	return hdbc != SQL_NULL_HANDLE;
}

void ODBCSession::Close()
{
	if(hdbc != SQL_NULL_HANDLE) {
		current = NULL;
		FlushConnections();
		SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
		hdbc = SQL_NULL_HANDLE;
		hstmt = SQL_NULL_HANDLE;
		current = NULL;
	}
}

void ODBCSession::FlushConnections()
{
	DLOG("FlushConnections");
	if(current) {
		current->Flush();
		current = NULL;
	}
	SQLFreeStmt(hstmt, SQL_CLOSE);
}

bool ODBCSession::IsOk(SQLRETURN ret)
{
	if(SQL_SUCCEEDED(ret))
		return true;
	SQLCHAR       SqlState[6], Msg[SQL_MAX_MESSAGE_LENGTH];
	SQLINTEGER    NativeError;
	SQLSMALLINT   MsgLen;
	String        error;
	int i = 1;
	while(SQLGetDiagRec(SQL_HANDLE_DBC, hdbc, i++, SqlState, &NativeError,
	                 Msg, sizeof(Msg), &MsgLen) != SQL_NO_DATA) {
		if(error.GetCount())
			error << "\r\n";
		error << (char *)Msg;
	}
	SetError(error, statement);
	return false;
}

ODBCSession::ODBCSession()
{
	hdbc = SQL_NULL_HANDLE;
	hstmt = SQL_NULL_HANDLE;
	SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
	if(henv)
		SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);
	tlevel = 0;
	Dialect(MSSQL);
	current = NULL;
}

ODBCSession::~ODBCSession()
{
	if(henv)
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
}

void ODBCSession::Begin()
{
	if(tlevel == 0)
		SQLSetConnectAttr(hdbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, SQL_NTS);
	tlevel++;
}

void ODBCSession::Commit()
{
	tlevel--;
	ASSERT(tlevel >= 0);
	if(tlevel == 0) {
		SQLEndTran(SQL_HANDLE_DBC, hdbc, SQL_COMMIT);
		SQLSetConnectAttr(hdbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, SQL_NTS);
	}
}

void ODBCSession::Rollback()
{
	tlevel--;
	ASSERT(tlevel >= 0);
	if(tlevel == 0) {
		SQLEndTran(SQL_HANDLE_DBC, hdbc, SQL_ROLLBACK);
		SQLSetConnectAttr(hdbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, SQL_NTS);
	}
}

String ODBCSession::Savepoint()
{
	NEVER();
	return "";
}

void ODBCSession::RollbackTo(const String& savepoint)
{
	NEVER();
}

Vector<String> ODBCSession::EnumUsers()
{
	return Vector<String>();
}

Vector<String> ODBCSession::EnumDatabases()
{
	return Vector<String>();
}

Vector<String> ODBCSession::EnumTables(String database)
{
	return Vector<String>();
}

Vector<String> ODBCSession::EnumViews(String database)
{
	return Vector<String>();
}

Vector<String> ODBCSession::EnumSequences(String database)
{
	return Vector<String>();
}

Vector<String> ODBCSession::EnumPrimaryKeys(String database, String table)
{
	return Vector<String>();
}

String ODBCSession::EnumRowID(String database, String table)
{
	return "";
}

bool   ODBCPerformScript(const String& text, StatementExecutor& executor, Gate2<int, int> progress_canceled)
{
	const char *p = text;
	while(*p) {
		String cmd;
		while(*p && *p != ';')
			if(*p == '\'') {
				const char *s = p;
				while(*++p && (*p != '\'' || *++p == '\''))
					;
				cmd.Cat(s, int(p - s));
			}
			else {
				if(*p > ' ')
					cmd.Cat(*p);
				else if(!cmd.IsEmpty() && *cmd.Last() != ' ')
					cmd.Cat(' ');
				p++;
			}
		if(progress_canceled(int(p - text.Begin()), text.GetLength()))
			return false;
		if(!IsNull(cmd) && !executor.Execute(cmd))
			return false;
		if(*p == ';')
			p++;
	}
	return true;
}

SqlConnection *ODBCSession::CreateConnection()
{
	return new ODBCConnection(this);
}

ODBCConnection::ODBCConnection(ODBCSession *session_)
:	session(session_)
{
	DLOG("ODBCConnection " << (void *)this << " " << (void *)session);
	rowcount = rowi = 0;
}

ODBCConnection::~ODBCConnection()
{
	if(IsCurrent())
		session->current = NULL;
	DLOG("~ODBCConnection " << (void *)this << " " << (void *)session);
}

bool ODBCConnection::IsOk(SQLRETURN ret) const
{
	if(SQL_SUCCEEDED(ret))
		return true;
	SQLCHAR       SqlState[6], Msg[SQL_MAX_MESSAGE_LENGTH];
	SQLINTEGER    NativeError;
	SQLSMALLINT   MsgLen;
	String        error;
	int i = 1;
	while(SQLGetDiagRec(SQL_HANDLE_STMT, session->hstmt, i++, SqlState, &NativeError,
	                    Msg, sizeof(Msg), &MsgLen) != SQL_NO_DATA) {
		if(error.GetCount())
			error << "\r\n";
		error << (char *)Msg;
	}
	session->SetError(error, statement);
	return false;
}

void ODBCConnection::SetParam(int i, const Value& r)
{
	Param& p = param.At(i);
	if(IsNumber(r)) {
		double x = r;
		p.ctype = SQL_C_DOUBLE;
		p.sqltype = SQL_DOUBLE;
		p.data = String((char *)&x, sizeof(x));
		p.li = sizeof(x);
	}
	if(IsString(r)) {
		p.ctype = SQL_C_CHAR;
		p.sqltype = SQL_LONGVARCHAR;
		p.data = r;
		p.li = p.data.GetLength();
	}
	if(IsDateTime(r)) {
		p.ctype = SQL_C_TYPE_TIMESTAMP;
		p.sqltype = SQL_TYPE_TIMESTAMP;
		Time t = r;
		SQL_TIMESTAMP_STRUCT tm;
		tm.year = t.year;
		tm.month = t.month;
		tm.day = t.day;
		tm.hour = t.hour;
		tm.minute = t.minute;
		tm.second = t.second;
		tm.fraction = 0;
		p.data = String((char *)&tm, sizeof(tm));
		p.li = sizeof(tm);
	}
	if(IsNull(r))
		p.li = SQL_NULL_DATA;
}

bool ODBCConnection::Execute()
{
	DLOG("Execute " << (void *)this << " " << (void *)session);
	if(session->hstmt == SQL_NULL_HANDLE)
		return false;
	if(IsCurrent())
		session->current = NULL;
	session->FlushConnections();
	last_insert_table.Clear();
	number.Clear();
	text.Clear();
	time.Clear();
	CParser p(statement);
	if((p.Id("insert") || p.Id("INSERT")) && (p.Id("into") || p.Id("INTO")) && p.IsId())
		last_insert_table = p.ReadId();
	if(!IsOk(SQLPrepare(session->hstmt, (SQLCHAR *)~statement, statement.GetCount())))
		return false;
	parse = false;
	bparam = param;
	param.Clear();
	for(int i = 0; i < bparam.GetCount(); i++) {
		Param& p = bparam[i];
		if(!IsOk(SQLBindParameter(session->hstmt, i + 1, SQL_PARAM_INPUT, p.ctype, p.sqltype,
		                          p.data.GetCount(), 0, (SQLPOINTER)~p.data, p.data.GetLength(),
		                          &p.li)))
			return false;
	}
	SQLSMALLINT ncol;
	if(!IsOk(SQLExecute(session->hstmt)) || !IsOk(SQLNumResultCols(session->hstmt, &ncol))) {
		SQLFreeStmt(session->hstmt, SQL_CLOSE);
		return false;
	}
	session->current = this;
	info.Clear();
	for(int i = 1; i <= ncol; i++) {
		SQLCHAR      ColumnName[256];
		SQLSMALLINT  NameLength;
		SQLSMALLINT  DataType;
		SQLULEN      ColumnSize;
		SQLSMALLINT  DecimalDigits;
		SQLSMALLINT  Nullable;
		if(!IsOk(SQLDescribeCol(session->hstmt, i, ColumnName, 255, &NameLength, &DataType,
		                        &ColumnSize, &DecimalDigits, &Nullable)))
			return false;
		SqlColumnInfo& f = info.Add();
		f.nullable = Nullable != SQL_NO_NULLS;
		f.precision = DecimalDigits;
		f.scale = 0;
		f.width = ColumnSize;
		f.name = (char *)ColumnName;
		switch(DataType) {
		case SQL_DECIMAL:
		case SQL_NUMERIC:
		case SQL_SMALLINT:
		case SQL_INTEGER:
		case SQL_REAL:
		case SQL_FLOAT:
		case SQL_DOUBLE:
		case SQL_BIT:
		case SQL_TINYINT:
		case SQL_BIGINT:
			f.type = DOUBLE_V;
			break;
		case SQL_TYPE_DATE:
		case SQL_TYPE_TIMESTAMP:
			f.type = TIME_V;
			break;
		default:
			f.type = STRING_V;
			break;
		}
	}
	SQLLEN rc;
	SQLRowCount(session->hstmt, &rc);
	rowsprocessed = rc;
	return true;
}

int ODBCConnection::GetRowsProcessed() const
{
	return rowsprocessed;
}

bool ODBCConnection::Fetch0()
{
	DLOG("Fetch0 " << (void *)this << " " << (void *)session);
	int ret = SQLFetch(session->hstmt);
	DDUMP(ret == SQL_NO_DATA);
	if(ret == SQL_NO_DATA || !IsOk(ret))
		return false;
	fetchrow.Clear();
	double dbl;
	SQL_TIMESTAMP_STRUCT tm;
	SQLLEN li;
	for(int i = 0; i < info.GetCount(); i++) {
		Value v = Null;
		switch(info[i].type) {
		case DOUBLE_V:
			if(!IsOk(SQLGetData(session->hstmt, i + 1, SQL_C_DOUBLE, &dbl, sizeof(dbl), &li)))
			   break;
			if(li != SQL_NULL_DATA)
				v = dbl;
			break;
		case TIME_V:
			if(!IsOk(SQLGetData(session->hstmt, i + 1, SQL_C_TYPE_TIMESTAMP, &tm, sizeof(tm), &li)))
			   break;
			if(li != SQL_NULL_DATA) {
				Time m;
				m.year = tm.year;
				m.month = (byte)tm.month;
				m.day = (byte)tm.day;
				m.hour = (byte)tm.hour;
				m.minute = (byte)tm.minute;
				m.second = (byte)tm.second;
				v = m;
			}
			break;
		default:
			if(!IsOk(SQLGetData(session->hstmt, i + 1, SQL_C_CHAR, &tm, 0, &li)))
			   break;
			if(li != SQL_NULL_DATA) {
				StringBuffer sb;
				sb.SetLength(li);
				if(!IsOk(SQLGetData(session->hstmt, i + 1, SQL_C_CHAR, ~sb, li + 1, &li)))
				   break;
				v = String(sb);
			}
			break;
		}
		fetchrow.Add(v);
	}
	return ret != SQL_NO_DATA && IsOk(ret);
}

bool ODBCConnection::Fetch()
{
	if(IsCurrent())
		return Fetch0();
	if(rowi >= rowcount)
		return false;
	fetchrow.Clear();
	for(int i = 0; i < info.GetCount(); i++) {
		Value v;
		switch(info[i].type) {
		case DOUBLE_V:
			v = number[i][rowi];
			break;
		case TIME_V:
			v = time[i][rowi];
			break;
		default:
			v = text[i][rowi];
			break;
		}
		fetchrow.Add(v);
	}
	++rowi;
	return true;
}

void ODBCConnection::GetColumn(int i, Ref r) const
{
	DLOG("GetColumn " << (void *)this << " " << (void *)session);
	r.SetValue(fetchrow[i]);
}

void ODBCConnection::Flush()
{
	DLOG("Flush " << (void *)this);
	rowcount = 0;
	rowi = 0;
	while(Fetch0()) {
		rowcount++;
		for(int i = 0; i < info.GetCount(); i++)
			switch(info[i].type) {
			case DOUBLE_V:
				number[i].Add(fetchrow[i]);
				break;
			case STRING_V:
				text[i].Add(fetchrow[i]);
				break;
			case TIME_V:
				time[i].Add(fetchrow[i]);
				break;
			}
	}
}

void ODBCConnection::Cancel()
{
	param.Clear();
	bparam.Clear();
	number.Clear();
	text.Clear();
	time.Clear();
}

String ODBCConnection::ToString() const
{
	return statement;
}

Value ODBCConnection::GetInsertedId() const
{
	Sql sql(GetSession());
	return last_insert_table.GetCount() ? sql.Select("IDENT_CURRENT('" + last_insert_table + "')")
	                                    : sql.Select("@@IDENTITY");
}

String MSSQLTextType(int width)
{
	if(width <= 4000)
		return NFormat("varchar(%d)", width);
	return "text";
}

END_UPP_NAMESPACE
