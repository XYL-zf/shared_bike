#ifndef NS_EVENT_H_
#define NS_EVENT_H_
#include"glo_def.h"
#include"eventtypes.h"
#include<string>
#include<iostream>
using namespace std;
class iEvent {
public:
	iEvent(u32 eid,u32 sn);
	
	virtual ~iEvent();
	virtual std::ostream& dump(ostream& out)const { return out; }
	virtual i32 ByteSize() { return 0; }
	virtual bool SerializeToArray(char* buf, int len) { return true; }
    u32 generateSeqNo();                 //����Ψһ�����к�
	u32 get_sn()const { return sn_; }    //��ȡ���к�	u32 get_eid()const { return eid_; }  //��ȡ�¼�ID


	void set_eid(u32 eid) {eid_ = eid;}  //�����¼�ID
	void* get_args() { return args_; }
	void set_args(void* args) { args_ = args; }
	u32 get_eid()const { return eid_; }
private:
	u32 eid_;
	u32 sn_;
	void* args_;
};

#endif
