#include"BusProcessor.h"
#include"SqlTables.h"

BussinessProcessor::BussinessProcessor(std::shared_ptr<MySqlConnection> conn)
	:mysqlconn_(conn), ueh_(new userEventHandler()) {
    
}


bool BussinessProcessor::init() {
    SqlTables tables(mysqlconn_);
    tables.CreateUserInfo();
    tables.CreateBikeTable();
    //�����ı�Ҳ���������ٺ���
    return true;
}


//�����ʱ������ʲ������ʱ�����ʲô
BussinessProcessor::~BussinessProcessor()
{
    ueh_.reset();//������ü������0���ͷ����������
}
