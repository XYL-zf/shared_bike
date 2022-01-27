#ifndef BRKS_BUS_USERM_HANDLER_H_
#define BRKS_BUS_USERM_HANDLER_H_
#include"glo_def.h"
#include"iEventHandler.h"
#include"events_def.h"
#include"threadpool/thread.h"

#include<string>
#include<map>
#include<memory>

class userEventHandler :public iEventHandler {
public:
	userEventHandler();
	virtual ~userEventHandler();
	virtual iEvent* handle(const iEvent* ev);

private:
	MobileCodeRspEv* handle_mobile_code_req(MobileCodeReqEv* ev);
	i32 code_gen();                         //������֤��ķ���
	LoginResEv* handle_login_req(LoginReqEv* ev);
private:
	 //std::string mobile_;
	//first is mobile second is code,��������һ����ϣ���������ֻ������Ӧ����֤��
	std::map<std::string, i32>m2c_; 
	pthread_mutex_t pm_;
};

#endif