#include"user_service.h"

UserService::UserService(std::shared_ptr<MySqlConnection>sql_conn) :sql_conn_(sql_conn) {
}

bool UserService::exists(std::string& mobile) {
    //дһ��sql���(�ڲ�ѯ���иú�����û��Ƿ��Ѿ��������ݿ��û�������)
    char sql_content[1024] = { 0 };

    sprintf(sql_content, "select * from userinfo where mobile = %s", mobile.c_str());
    SqlRecordSet record_set;//���淵�ؽ��

    if (!sql_conn_->Execute(sql_content, record_set)) {
        return false;
    }

    return(record_set.GetRowCount() != 0);
      
}

bool UserService::insert(std::string& mobile) {
    char sql_content[1024] = { 0 };

    sprintf(sql_content, "insert into userinfo (mobile) values (%s)", mobile.c_str());

    return sql_conn_->Execute(sql_content);
}