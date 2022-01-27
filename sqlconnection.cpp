#include"sqlconnection.h"
#include"string.h"

MySqlConnection::MySqlConnection():mysql_(NULL) {

	mysql_ = (MYSQL*)malloc(sizeof(MYSQL));
}

MySqlConnection::~MySqlConnection() {

	if (mysql_) {
	
		mysql_close(mysql_);
		free(mysql_);
		mysql_ = NULL;

	}
}

bool MySqlConnection::Init(const char* szHost, int nPort, const char* szUser, const char* szPasswd, const char* szDb)
{
	//1.初始化
	mysql_ = mysql_init(mysql_);
	if (mysql_ == NULL) {
		LOG_ERROR("init mysql failed %s,%d\n", this->GetErrInfo(), errno);
		return false;
	}
	
	//设置选项，如果失败允许重连
	char cAuto = 1;

	if (mysql_options(mysql_, MYSQL_OPT_RECONNECT, &cAuto)!=0) {
		LOG_ERROR("mysql_options MYSQL_OPT_RECONNECT failed! reason:%s",GetErrInfo());
	}

	//2.连接数据库
	if (mysql_real_connect(mysql_, szHost, szUser, szPasswd, szDb, nPort, NULL, 0) == NULL) {
		LOG_ERROR("connect mysql failed %s", this->GetErrInfo());
		return false;
	}

	return true;
}


//执行mysql语句
bool MySqlConnection::Execute(const char* szSql)
{
	if (mysql_real_query(mysql_, szSql, strlen(szSql)!=0)) {
		if (mysql_errno(mysql_) == CR_SERVER_GONE_ERROR) {
			Reconnect();    
		}
		return false;
	}
	return true;
}


int MySqlConnection::EscapeString(const char* pSrc, int SrcLen, char* pDest)
{
	if (!mysql_)return 0;
	return mysql_real_escape_string(mysql_,pDest, pSrc, SrcLen);
}

const char* MySqlConnection::GetErrInfo()
{
	return mysql_error(mysql_);
}

void MySqlConnection::Reconnect()
{
	mysql_ping(mysql_);//执行重连
}

bool MySqlConnection::Execute(const char* szSql, SqlRecordSet& recordSet)
{
	if (mysql_real_query(mysql_, szSql, strlen(szSql) != 0)) {
		if (mysql_errno(mysql_) == CR_SERVER_GONE_ERROR) {
		    //重连
			Reconnect();
		}
		return false;
	}
	
	//获取查询结果集
	MYSQL_RES* pRes = mysql_store_result(mysql_);
	if (!pRes) {
		return NULL;
	}
	recordSet.SetResult(pRes);//保存结果集，以后需要就在recordSet里面取
	return true;
}


