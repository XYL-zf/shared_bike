#include"BusProcessor.h"
#include"SqlTables.h"

BussinessProcessor::BussinessProcessor(std::shared_ptr<MySqlConnection> conn)
	:mysqlconn_(conn), ueh_(new userEventHandler()) {
    
}


bool BussinessProcessor::init() {
    SqlTables tables(mysqlconn_);
    tables.CreateUserInfo();
    tables.CreateBikeTable();
    //后续的表也可以增加再后面
    return true;
}


//构造的时候做了什析构的时候就做什么
BussinessProcessor::~BussinessProcessor()
{
    ueh_.reset();//如果引用计数变成0就释放这个对象了
}
