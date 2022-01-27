/*****************************************
* ����ַ���Ϣ����ģ�飬��ʼ���ǰ��ⲿ�յ�����Ϣת�����ڲ��¼���Ҳ���� data->msg->event�Ľ������
* Ȼ���ٰ��¼�Ͷ�ݵ��̳߳ص��е���Ϣ���У��ȴ��̳߳ص�����process���������¼�
* ���յ���ÿ��event��handler����������event����ʱÿ��event handler ��Ҫsubscribe�Żᱻ���õ�
****************************************/
#ifndef BRK_SERVICE_DISPATCH_EVENT_SERVICE_H_
#define BRK_SERVICE_DISPATCH_EVENT_SERVICE_H_

#include <map>
#include <vector>
#include "ievent.h"
#include "eventtypes.h"
#include "iEventHandler.h"
#include "threadpool/thread_pool.h"
#include"NetworkInterface.h"
#include<queue>
#include"Logger.h"

class DispatchMsgService {
protected:
	DispatchMsgService();
public:
	
	virtual ~DispatchMsgService();
	virtual BOOL open();          //�򿪷���
	virtual void close();         //�رշ���
	virtual void subscribe(u32 eid,iEventHandler *handler);     //�����¼�
	virtual void unsubscribe(u32 eid, iEventHandler* handler);  //�˶��¼�
	virtual i32 enqueue(iEvent* ev);                            //���¼�Ͷ�ݵ��̳߳��з����д���
	static void svc(void* arg);                                 //�̳߳ػص�����
	virtual iEvent* process (const iEvent* ev);                  //�Ծ�����¼����зַ�����
	static DispatchMsgService* getInstance();                  //����
	iEvent* parseEvent(const char* message, u32 len, u32 eid);  //�����¼�
	void handleAllResponseEvent(NetworkInterface *interface);

protected:
	thread_pool_t *tp;
	static NetworkInterface* NTIF_;
	static DispatchMsgService* DMG_;
	typedef std::vector<iEventHandler*> T_EventHandlers;
	typedef std::map<u32, T_EventHandlers> T_EventHandlersMap;
	T_EventHandlersMap subscribers_;
	
	bool ser_exit; //����open()��������������ʼ������close()����ر�
	static queue<iEvent*>response_events; //��������̳߳ش���ô���Ӧ���ͻ��˵��¼�
	static pthread_mutex_t queue_mutex;  //���������еģ��������߳�ͬʱ������ɾ�̬

};

#endif