#ifndef BRKS_COMMON_EVENTS_DEF_H_
#define BRKS_COMMON_EVENTS_DEF_H_

#include<string>
#include<iostream>
#include"ievent.h"
#include"bike.pb.h"



//�ֻ���֤������
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
		//code����Ӧ����  icode:��֤��
		//���ݴ���Ĵ�����������Ҷ�Ӧ��������Ϣ��ÿһ��code��Ӧһ����������
		msg_.set_code(code);
		msg_.set_icode(icode);
		msg_.set_data(getReasonByErrorCode(code));
		
	}
	const i32 get_code() { return msg_.code(); }   //��ȡ�������
	const i32 get_icode() { return msg_.icode(); } //��ȡ��֤��
	const std::string& getdesc() { return msg_.data(); } //��ȡ��������

	virtual std::ostream& dump(ostream& out)const;
	virtual i32 ByteSize() { return msg_.ByteSize(); }
	virtual bool SerializeToArray(char* buf, int len) { return msg_.SerializeToArray(buf, len);}

private:
	tutorial::mobile_response msg_;   //��Ӧ
};

class ExitRspEv :public iEvent {
public:
	ExitRspEv() :iEvent(EEVENTID_EXIT_RSP, iEvent::generateSeqNo()) {

	}
};


//��½������
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

//��½��Ӧ
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