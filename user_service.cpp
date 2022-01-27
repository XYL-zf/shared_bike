#include"user_service.h"

UserService::UserService(std::shared_ptr<MySqlConnection>sql_conn) :sql_conn_(sql_conn) {
}

bool UserService::exists(std::string& mobile) {
    //写一条sql语句(于查询持有该号码的用户是否已经存在数据库用户表中用)
    char sql_content[1024] = { 0 };

    sprintf(sql_content, "select * from userinfo where mobile = %s", mobile.c_str());
    SqlRecordSet record_set;//保存返回结果

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