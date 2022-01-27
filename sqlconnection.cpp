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
	//1.��ʼ��
	mysql_ = mysql_init(mysql_);
	if (mysql_ == NULL) {
		LOG_ERROR("init mysql failed %s,%d\n", this->GetErrInfo(), errno);
		return false;
	}
	
	//����ѡ����ʧ����������
	char cAuto = 1;

	if (mysql_options(mysql_, MYSQL_OPT_RECONNECT, &cAuto)!=0) {
		LOG_ERROR("mysql_options MYSQL_OPT_RECONNECT failed! reason:%s",GetErrInfo());
	}

	//2.�������ݿ�
	if (mysql_real_connect(mysql_, szHost, szUser, szPasswd, szDb, nPort, NULL, 0) == NULL) {
		LOG_ERROR("connect mysql failed %s", this->GetErrInfo());
		return false;
	}

	return true;
}


//ִ��mysql���
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
	mysql_ping(mysql_);//ִ������
}

bool MySqlConnection::Execute(const char* szSql, SqlRecordSet& recordSet)
{
	if (mysql_real_query(mysql_, szSql, strlen(szSql) != 0)) {
		if (mysql_errno(mysql_) == CR_SERVER_GONE_ERROR) {
		    //����
			Reconnect();
		}
		return false;
	}
	
	//��ȡ��ѯ�����
	MYSQL_RES* pRes = mysql_store_result(mysql_);
	if (!pRes) {
		return NULL;
	}
	recordSet.SetResult(pRes);//�����������Ժ���Ҫ����recordSet����ȡ
	return true;
}


