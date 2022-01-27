#ifndef BRKS_COMMON_EVENTS_DEF_H_
#define BRKS_COMMON_EVENTS_DEF_H_

#include<string>
#include<iostream>
#include"ievent.h"
#include"bike.pb.h"



//手机验证请求类
class MobileCodeReqEv :public iEvent {

public:
	MobileCodeReqEv(const std::string& mobile) :iEvent(EEVENTID_GET_MOBILE_CODE_REQ, iEvent::generateSeqNo())
	{
		_msg.set_mobile(mobile);
	}
	const std::string& get_mobile() {
		return _msg.mobile();
	}

	virtual std::ostream& dump(std::ostream& out)const;
	virtual i32 ByteSize() { return _msg.ByteSize(); }
	virtual bool SerializeToArray(char* buf, int len) { return _msg.SerializeToArray(buf, len); }

private:
	tutorial::mobile_request _msg;
};


class MobileCodeRspEv :public iEvent {

public:
	MobileCodeRspEv(i32 code, i32 icode) :iEvent(EEVENTID_GET_MOBILE_CODE_RSP, iEvent::generateSeqNo())
	{
		//code：响应代号  icode:验证码
		//根据传入的错误代号来查找对应的描述信息，每一个code对应一个错误描述
		msg_.set_code(code);
		msg_.set_icode(icode);
		msg_.set_data(getReasonByErrorCode(code));
		
	}
	const i32 get_code() { return msg_.code(); }   //获取错误代号
	const i32 get_icode() { return msg_.icode(); } //获取验证码
	const std::string& getdesc() { return msg_.data(); } //获取错误描述

	virtual std::ostream& dump(ostream& out)const;
	virtual i32 ByteSize() { return msg_.ByteSize(); }
	virtual bool SerializeToArray(char* buf, int len) { return msg_.SerializeToArray(buf, len);}

private:
	tutorial::mobile_response msg_;   //响应
};

class ExitRspEv :public iEvent {
public:
	ExitRspEv() :iEvent(EEVENTID_EXIT_RSP, iEvent::generateSeqNo()) {

	}
};


//登陆请求类
class LoginReqEv:public iEvent {
public:
	LoginReqEv(const std::string& mobile, i32 icode) :iEvent(EEVENTID_LOGIN_REQ, iEvent::generateSeqNo()) {
		msg_.set_mobile(mobile);
		msg_.set_icode(icode);
	}
	const string& get_mobile() {
		return msg_.mobile();
	}

	const i32 get_icode() {
		return msg_.icode();
	}

	virtual std::ostream& dump(std::ostream& out)const;
	virtual i32 ByteSize() { return  msg_.ByteSize(); }
	virtual bool SerializeToArray(char* buf, int len) {
		return msg_.SerializeToArray(buf, len);
	}

private:
	tutorial::login_request msg_;
};

//登陆响应
class LoginResEv :public iEvent {
public:
	LoginResEv(i32 code) :iEvent(EEVENTID_LOGIN_RSP, iEvent::generateSeqNo()) {
		msg_.set_code(code);
		msg_.set_desc(getReasonByErrorCode(code));
	}

	const i32 get_code() { return msg_.code(); }
	const std::string& get_desc() { return msg_.desc(); }
	virtual std::ostream& dump(std::ostream& out)const;
	virtual i32 ByteSize() { return  msg_.ByteSize(); }
	virtual bool SerializeToArray(char* buf, int len) {
		return msg_.SerializeToArray(buf, len);
	}
private:
	
	tutorial::login_response msg_;
};

#endif