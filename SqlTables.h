#ifndef BRKS_COMMON_DATASERVER_SQLTABLES
#define BRKS_COMMON_DATASERVER_SQLTABLES

#include<memory>
#include"sqlconnection.h"
#include"glo_def.h"

class SqlTables {
public:
	SqlTables(std::shared_ptr<MySqlConnection>sqlconn) :sqlconn_(sqlconn) { }

	//创建用户表
	bool CreateUserInfo() {
        const char* pUserInfoTable = "\
		            CREATE TABLE IF NOT EXISTS userinfo( \
                    id		    int(16)	        NOT NULL primary key   auto_increment, \
                    mobile      varchar(16)     NOT NULL default       '1300000000',   \
                    username    varchar(128)    NOT NULL default         '',           \
                    verify      int(4)          NOT NULL default         '0',            \
                    registerm   timestamp       NOT NULL default   CURRENT_TIMESTAMP,    \
                    money       int(4)          NOT NULL default         0,\
                    INDEX       mobile_index(mobile)                      \
                )";
        if (!sqlconn_->Execute(pUserInfoTable)) {
            LOG_ERROR("create user info failed ,reason:%s", sqlconn_->GetErrInfo());
            return false;
        }
        return true;
	}

    //创建单车表

    bool CreateBikeTable() {
        const char* pBikeInfoTable = "\
                    CREATE TABLE IF NOT EXISTS bikeinfo( \
                    id         int              NOT NULL primary key   auto_increment,\
                    devno      int              NOT NULL , \
                    status     tinyint(1)       NOT NULL default 0,\
                    trouble    int              NOT NULL default 0,\
                    tmsg       varchar(256)     NOT NULL default '',\
                    latitude   double(10,6)     NOT NULL default 0, \
                    longitude  double(10,6)     NOT NULL default 0,  \
                    unique(devno)   \
            )";

        if (!sqlconn_->Execute(pBikeInfoTable)) {
            LOG_ERROR("create bike info failed ,reason:%s", sqlconn_->GetErrInfo());
            return false;
        }
        return true;
    }
private:
	std::shared_ptr<MySqlConnection> sqlconn_;//共享指针
};

#endif

