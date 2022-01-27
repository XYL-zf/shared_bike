#ifndef DATASTORE_MYSQL_CONNECTION_H
#define DATASTORE_MYSQL_CONNECTION_H

#include<mysql/mysql.h>
#include<mysql/errmsg.h>
#include<assert.h>
#include<string>
#include"glo_def.h"

class SqlRecordSet {
public:
	SqlRecordSet() :m_pRes(NULL) {
	     
	}

	explicit SqlRecordSet(MYSQL_RES* Res) {
		m_pRes = Res;
	}

	MYSQL_RES* MySqlRes() {
		return m_pRes;
	}

	~SqlRecordSet() {
		if (m_pRes) {
			mysql_free_result(m_pRes);
		}
	}

	inline void SetResult(MYSQL_RES*pRes) {
		assert(m_pRes == NULL);//如果此时已经保存结果集了，那么应该让程序报错，防止内存泄露
		if (m_pRes) {
		    LOG_WARN("THE MYSQL_RRES has already stores reuslt，maby will cause memory leak你\n");
		}
		m_pRes = pRes;
	}

	inline MYSQL_RES* getResult() {
		return m_pRes;
	}

	void FetchRow(MYSQL_ROW& row) {
		row = mysql_fetch_row(m_pRes);
	}

	inline i32 GetRowCount() {
		return m_pRes->row_count;
	}
private:
	MYSQL_RES* m_pRes;
	   

};

class MySqlConnection {
public:
	MySqlConnection();
	~MySqlConnection();
	MYSQL* Mysql() {
		return mysql_;
	}

	bool Init(const char* szHost, int nPort, const char* szUser, const char* szPasswd, const char* szDb);
	bool Execute(const char* szSql);
	bool Execute(const char* szSql, SqlRecordSet& recordSet);//MYSQL_RES*//结果集
	void close();
	int EscapeString(const char*pSrc,int SrcLen,char* pDest);
	const char* GetErrInfo();
	void Reconnect();
private:

	MYSQL* mysql_;
};


#endif
