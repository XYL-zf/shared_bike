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
	//���Ķ�����֤����
	DispatchMsgService::getInstance()->subscribe(EEVENTID_GET_MOBILE_CODE_REQ, this);
	//���ĵ�½�¼�
	DispatchMsgService::getInstance()->subscribe(EEVENTID_LOGIN_REQ, this);
	DispatchMsgService::getInstance()->subscribe(EEVENTID_GET_MOBILE_CODE_RSP, this);

	thread_mutex_create(&pm_);
}

userEventHandler::~userEventHandler()
{
	//�˶��¼�
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

	//�ж��¼�������
	if (eid == EEVENTID_GET_MOBILE_CODE_REQ) {//��½����
		return handle_mobile_code_req((MobileCodeReqEv*)ev);
	}
	else if (eid == EEVENTID_GET_MOBILE_CODE_RSP) {//��½�����¼�
		//return handle_login_req((LoginReqEv*)ev);
		return handle_mobile_code_req((MobileCodeReqEv*)ev);
	}
	else if (eid == EEVENTID_LOGIN_REQ) {
		return handle_login_req((LoginReqEv*)ev);
	}

	else if (eid == EEVENTID_GET_ACCOUNT_BALANCE_REQ) {
	    //����ѯ�¼�
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
    //�����¼��ĵ绰���롣����һ���������֤��
	std::string mobile_ = ev->get_mobile();//ȡ���¼����ֻ�����
	printf("try to get mobile phone validate code  of %s\n", mobile_.c_str());
	icode = code_gen();//������������һ���������֤��
	thread_mutex_lock(&pm_);
	m2c_[mobile_] = icode;
	thread_mutex_unlock(&pm_); 
	//������������
	printf("phone number:%s\tvalidate code:%d\n", mobile_.c_str(), icode);
	return new MobileCodeRspEv(200, m2c_[mobile_]);
}

//ʵ��code_gen()����

i32 userEventHandler::code_gen() {
	i32 code = 0;
	//�����������
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
	//���¼���ĵ绰�����m2c��ĵ绰����Ƚϣ����һ���������½�����򷵻���Ч�¼�

	thread_mutex_lock(&pm_);
	auto iter = m2c_.find(mobile);
	if (iter != m2c_.end() && iter->second != code || iter == m2c_.end())
	{
		login = new LoginResEv(ERRC_INVALID_DATA);
	}
	if (login)return login;
	thread_mutex_unlock(&pm_);

	//��֤�ɹ��Ժ���жϸ��û��Ӱ����ݿ��������ڣ�����������½������������������ݿ���
	//�����û���¼

	//��Ҫ�������������MySqlConnection�����ӣ���Ϊ������Ҫ�ܺܶ���̵߳��õģ�����߳�̫��
	//������ɺܶ����벻��������
	std::shared_ptr<MySqlConnection>msqlconn(new MySqlConnection);
	st_env_config conf_args = Iniconfig::getInstance()->getconfig();
	if (!msqlconn->Init(conf_args.db_ip.c_str(), conf_args.db_port,
		conf_args.db_user.c_str(), conf_args.db_pwd.c_str(), conf_args.db_name.c_str())) {
		LOG_DEBUG("userEventHandler::handle_login_req - connect database failed!\n");
		return new LoginResEv(ERRO_PROCCESS_FAILED);
	}
	//���ӳɹ�
	UserService us(msqlconn);
	//�жϵ�ǰ�û����ֻ����������ݿ����Ƿ����
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
