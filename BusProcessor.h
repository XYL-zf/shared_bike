#ifndef BRKS_BUS_MAIN_H
#define BRKS_BUS_MAIN_H

#include"user_event_handler.h"
#include"sqlconnection.h"


class BussinessProcessor {
public:
    BussinessProcessor(std::shared_ptr<MySqlConnection> conn);
	bool init();     
	virtual ~BussinessProcessor();
private:

	std::shared_ptr<MySqlConnection>mysqlconn_;
	std::shared_ptr<userEventHandler>ueh_;//账户管理系统

};
#endif
