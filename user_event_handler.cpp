#include"user_event_handler.h"
#include"DispatchMsgService.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <unistd.h>
#include"sqlconnection.h"
#include"configdef.h"
#include"iniconfig.h"
#include"user_service.h"


userEventHandler::userEventHandler() :iEventHandler("iEventHandler")
{
	//订阅短信验证请求
	DispatchMsgService::getInstance()->subscribe(EEVENTID_GET_MOBILE_CODE_REQ, this);
	//订阅登陆事件
	DispatchMsgService::getInstance()->subscribe(EEVENTID_LOGIN_REQ, this);
	DispatchMsgService::getInstance()->subscribe(EEVENTID_GET_MOBILE_CODE_RSP, this);

	thread_mutex_create(&pm_);
}

userEventHandler::~userEventHandler()
{
	//退订事件
	DispatchMsgService::getInstance()->unsubscribe(EEVENTID_GET_MOBILE_CODE_REQ, this);
	DispatchMsgService::getInstance()->unsubscribe(EEVENTID_LOGIN_REQ, this);
	thread_mutex_destroy(&pm_);
}


/****************************************************************
* 
	EEVENTID_GET_MOBILE_CODE_REQ        = 0x01,
	EEVENTID_GET_MOBILE_CODE_RSP        =0x02,

	EEVENTID_LOGIN_REQ                  =0x03,
	EEVENTID_LOGIN_RSP                  =0x04,

	EEVENTID_RECHARGE_REQ               =0x05,
	EEVENTID_RECHARGE_RSP               =0x06,

	EEVENTID_GET_ACCOUNT_BALANCE_REQ    =0x07,
	EEVENTID_ACCOUNT_BALANCE_RSP        =0x8,

	EEVENTID_LIST_ACCOUNT_RECORDS_REQ   =0x09,
	EEVENTID_ACCOUNT_RECORDS_RSP        =0x10,

	EEVENTID_LIST_TRAVELS_REQ           =0x11,
	EEVENTID_LIST_TRAVELS_RSP           =0x12,

	EEVENTID_UNKOWN                     =0xFF
*****************************************************************/
iEvent* userEventHandler::handle(const iEvent* ev) {

	if (ev == NULL) {
		printf("input ev is NULL !\n");
	}

	u32 eid = ev->get_eid();

	//判断事件的类型
	if (eid == EEVENTID_GET_MOBILE_CODE_REQ) {//登陆请求
		return handle_mobile_code_req((MobileCodeReqEv*)ev);
	}
	else if (eid == EEVENTID_GET_MOBILE_CODE_RSP) {//登陆请求事件
		//return handle_login_req((LoginReqEv*)ev);
		return handle_mobile_code_req((MobileCodeReqEv*)ev);
	}
	else if (eid == EEVENTID_LOGIN_REQ) {
		return handle_login_req((LoginReqEv*)ev);
	}

	else if (eid == EEVENTID_GET_ACCOUNT_BALANCE_REQ) {
	    //余额查询事件
		//return handle_account_balance_req((Account_balanceEv*)ev);
	}
	else if (eid == EEVENTID_LIST_ACCOUNT_RECORDS_REQ) {
		//return handle_list_account_records_req((ListAccountRecordsReqEv*)ev);

	}
	return NULL;
}

MobileCodeRspEv* userEventHandler::handle_mobile_code_req(MobileCodeReqEv* ev) {
	LOG_DEBUG("userEventHandler::handle_mobile_code_req - eid\n", ev->get_eid());
	i32 icode = 0;
    //根据事件的电话号码。产生一个随机的验证码
	std::string mobile_ = ev->get_mobile();//取得事件的手机号码
	printf("try to get mobile phone validate code  of %s\n", mobile_.c_str());
	icode = code_gen();//给这个号码分配一个随机的验证码
	thread_mutex_lock(&pm_);
	m2c_[mobile_] = icode;
	thread_mutex_unlock(&pm_); 
	//将处理结果返回
	printf("phone number:%s\tvalidate code:%d\n", mobile_.c_str(), icode);
	return new MobileCodeRspEv(200, m2c_[mobile_]);
}

//实现code_gen()方法

i32 userEventHandler::code_gen() {
	i32 code = 0;
	//配置随机种子
	srand((unsigned int)time(NULL));
	code = (unsigned int)(rand()%(999999 - 100000) + 100000);
	return code;
}

LoginResEv* userEventHandler::handle_login_req(LoginReqEv* ev)
{
	LoginResEv* login = nullptr;
 	std::string mobile = ev->get_mobile();
	i32 code = ev->get_icode();
	LOG_DEBUG("try to handle login ev, icode = %d\tmobile = %s", code, mobile.c_str());
	//将事件里的电话号码和m2c里的电话号码比较，如果一样则允许登陆，否则返回无效事件

	thread_mutex_lock(&pm_);
	auto iter = m2c_.find(mobile);
	if (iter != m2c_.end() && iter->second != code || iter == m2c_.end())
	{
		login = new LoginResEv(ERRC_INVALID_DATA);
	}
	if (login)return login;
	thread_mutex_unlock(&pm_);

	//验证成功以后就判断该用户子啊数据库中书否存在，如果存在则登陆，如果不存在则再数据库中
	//插入用户记录

	//不要共享主进程里的MySqlConnection的连接，因为这里是要受很多的线程调用的，如果线程太多
	//容易造成很多意想不到的问题
	std::shared_ptr<MySqlConnection>msqlconn(new MySqlConnection);
	st_env_config conf_args = Iniconfig::getInstance()->getconfig();
	if (!msqlconn->Init(conf_args.db_ip.c_str(), conf_args.db_port,
		conf_args.db_user.c_str(), conf_args.db_pwd.c_str(), conf_args.db_name.c_str())) {
		LOG_DEBUG("userEventHandler::handle_login_req - connect database failed!\n");
		return new LoginResEv(ERRO_PROCCESS_FAILED);
	}
	//连接成功
	UserService us(msqlconn);
	//判断当前用户的手机号码再数据库中是否存在
	bool result = false;
	if (!us.exists(mobile)) {
		result = us.insert(mobile);
		if (!result) {
			LOG_DEBUG(" userEventHandler::handle_login_req - inser user(%s) to db failed\n", mobile.c_str());
			return new LoginResEv(ERRO_PROCCESS_FAILED);
		}
	}

	return new LoginResEv(ERRC_SUCCESS);
}
