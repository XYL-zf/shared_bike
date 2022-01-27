#ifndef BRKS_SVR_USER_SERVICE_H
#define BRKS_SVR_USER_SERVICE_H

#include"glo_def.h"
#include"sqlconnection.h"
#include"events_def.h"
#include<memory>

class UserService {
public:
	UserService(std::shared_ptr<MySqlConnection>sql_conn);
	bool exists(std::string& mobile);//�ж��ֻ��Ƿ���ڣ������������һ����¼
	bool insert(std::string& mobile);// ����һ����¼�Ľӿ�
private:
	std::shared_ptr<MySqlConnection> sql_conn_;
};

#endif