#ifndef BRKS_SVR_USER_SERVICE_H
#define BRKS_SVR_USER_SERVICE_H

#include"glo_def.h"
#include"sqlconnection.h"
#include"events_def.h"
#include<memory>

class UserService {
public:
	UserService(std::shared_ptr<MySqlConnection>sql_conn);
	bool exists(std::string& mobile);//判断手机是否存在，不存在则插入一条记录
	bool insert(std::string& mobile);// 插入一条记录的接口
private:
	std::shared_ptr<MySqlConnection> sql_conn_;
};

#endif