/*****************************************
* 负责分发消息服务模块，起始就是把外部收到的消息转化成内部事件，也就是 data->msg->event的解码过程
* 然后再把事件投递到线程池的中的消息队列，等待线程池调用其process方法处理事件
* 最终调用每个event的handler方法来处理event，此时每个event handler 需要subscribe才会被调用到
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
	virtual BOOL open();          //打开服务
	virtual void close();         //关闭服务
	virtual void subscribe(u32 eid,iEventHandler *handler);     //订阅事件
	virtual void unsubscribe(u32 eid, iEventHandler* handler);  //退订事件
	virtual i32 enqueue(iEvent* ev);                            //将事件投递到线程池中方进行处理
	static void svc(void* arg);                                 //线程池回调函数
	virtual iEvent* process (const iEvent* ev);                  //对具体的事件进行分发处理
	static DispatchMsgService* getInstance();                  //单例
	iEvent* parseEvent(const char* message, u32 len, u32 eid);  //解析事件
	void handleAllResponseEvent(NetworkInterface *interface);

protected:
	thread_pool_t *tp;
	static NetworkInterface* NTIF_;
	static DispatchMsgService* DMG_;
	typedef std::vector<iEventHandler*> T_EventHandlers;
	typedef std::map<u32, T_EventHandlers> T_EventHandlersMap;
	T_EventHandlersMap subscribers_;
	
	bool ser_exit; //调用open()服务启动，服务开始，调用close()服务关闭
	static queue<iEvent*>response_events; //用来存放线程池处理好待响应给客户端的事件
	static pthread_mutex_t queue_mutex;  //用于锁队列的，避免多个线程同时访问造成竟态

};

#endif