
#include"algorithm"
#include"DispatchMsgService.h"
#include"bike.pb.h"
#include"events_def.h"


queue<iEvent*> DispatchMsgService::response_events;
pthread_mutex_t DispatchMsgService::queue_mutex;
 DispatchMsgService* DispatchMsgService::DMG_ = NULL;

DispatchMsgService::DispatchMsgService() {
	tp = nullptr;
}

DispatchMsgService::~DispatchMsgService() {
    
}

BOOL DispatchMsgService::open() {
    ser_exit = FALSE;
    tp = thread_pool_init();
    thread_mutex_create(&queue_mutex);
    return tp ? TRUE : FALSE;
}

void DispatchMsgService::close() {
    ser_exit = TRUE;
    thread_pool_destroy(tp);
    thread_mutex_destroy(&queue_mutex);
    subscribers_.clear();
    tp = NULL;
}

DispatchMsgService* DispatchMsgService::getInstance()
{
    if (DMG_ == nullptr) {
        DMG_ = new DispatchMsgService;
    }

    return DMG_;
}

iEvent* DispatchMsgService::parseEvent(const char* message, u32 len, u32 eid)
{
    //LOG_DEBUG("DispatchMsgService::parseEvent - eid = %d\n", eid);
    if (!message) {
        LOG_ERROR("DispatchMsgService::parseEvent - message is null,eid : %d", eid);
        return nullptr;
    }
    //解析事件
    if (eid == EEVENTID_GET_MOBILE_CODE_REQ) {
        //LOG_DEBUG("DispatchMsgService::parseEvent - eid : %d\n", eid);
        tutorial::mobile_request mr;
        if (mr.ParseFromArray(message, len)) {
            MobileCodeReqEv* ev = new MobileCodeReqEv(mr.mobile());
            return ev;
        }
    }
    else if (eid == EEVENTID_LOGIN_REQ) {
        tutorial::login_request lr;
        if (lr.ParseFromArray(message, len)) {
            LoginReqEv* ev = new LoginReqEv(lr.mobile(), lr.icode());
            return ev;
        }
    }
    return nullptr;
}

void DispatchMsgService::svc(void* arg) {
    DispatchMsgService* dmg = DispatchMsgService::getInstance();

    iEvent* ev = (iEvent*)arg;

    if (!dmg->ser_exit) {
        LOG_DEBUG("DispatchMsgService sv... eid = %d\n",ev->get_eid());
        //接收到返回事件，将结果入队，等待线程池响应给客户端
        iEvent *rsp = dmg->process(ev);
       
        if (rsp) {
            rsp->dump(std::cout);
            rsp->set_args(ev->get_args());
        }
        else {
            //生成终止响应事件
            rsp = new ExitRspEv();
            rsp->set_args(ev->get_args());
        }
        //将rsp入队，注意：rsp入队前一定要上锁，因为有多个线程会同时访问这个队列
        thread_mutex_lock(&queue_mutex);
        response_events.push(rsp);
        //LOG_DEBUG("DispatchMsgService::svc ev has been in queue,,rsp eid = %d\n", rsp->get_eid());
        thread_mutex_unlock(&queue_mutex);
    }

}

iEvent* DispatchMsgService::process(const iEvent* ev)
{
    if (ev == NULL) {
        LOG_DEBUG("ev is NULL...\n");
        return NULL;
    }
    u32 eid = ev->get_eid();
    LOG_DEBUG("DispatchMsgService:process - eid: %u\n", eid);
    if (eid == EEVENTID_UNKOWN) {
        LOG_WARN("DispatchMsgService:Unkow event id:%d\n", eid);
        return NULL;
    }
    
    //检查该事件是否被订阅了
    T_EventHandlersMap::iterator handlers = subscribers_.find(eid);
    if (handlers == subscribers_.end()) {
        LOG_WARN("DispatchMsgService:no event handler subscribe %d\n", eid);
        return NULL;
    }
    //该事件有被订阅, 查找订阅该事件的处理器
    iEvent* rsp = NULL;
    for (auto iter = handlers->second.begin(); iter != handlers->second.end(); iter++) {
        iEventHandler* handler = *iter;       //*iter是继承自iEventHandler类，用父类指针做统一性处理
        LOG_DEBUG("DispatchMsgService:get handler name:%s\n", handler->getname().c_str());
        rsp = handler->handle(ev);
        //若同一个事件有多个订阅者,那就将函数的返回类型改成vector<iEvent*>类型
        //每当一个订阅者处理完一个事件，就将其处理结果加入vector<iEvnet*>中
        //最后返回这个数组就完事儿了
    }
    
    return rsp;
}


i32 DispatchMsgService::enqueue(iEvent* ev) {
    if (ev == NULL)return -1;
   // LOG_DEBUG("DispatchMsgService::enqueue - ev will be in queue... eid = %d\n", ev->get_eid());
    //分配一个任务
    thread_task_t* task = thread_task_alloc(0);
    task->ctx = ev;
    task->handler = DispatchMsgService::svc;

    //投递任务
    return thread_task_post(tp, task);
}


//订阅事件，指定该事件被哪个事件处理器处理
void DispatchMsgService::subscribe(u32 eid, iEventHandler* handler) {
    //一个事件可以绑定多个事件处理器
    //首先事件来了我们先查询一下该事件有没有之前就被其他处理器订阅，如果
    LOG_DEBUG("DispatchMsgService::subscribe eid:%u\n", eid);
    T_EventHandlersMap::iterator iter = subscribers_.find(eid);//查找容器中是否有该事件id
    if (iter != subscribers_.end()) {//该事件已经存在
        //接下来就查找该处理器是否再该容器中，如果如果不在就将该处理器添加进去，
        T_EventHandlers::iterator hd_iter = std::find(iter->second.begin(), iter->second.end(), handler);
        if (hd_iter == iter->second.end()) {//该容器中没有handler处理器
            iter->second.push_back(handler);//直接将handler添加到表中
        }
    }
    else {
        //该事件不存在
        subscribers_[eid].push_back(handler);    //将该事件与响应的处理器绑定即可
    }
}


//退订
void  DispatchMsgService::unsubscribe(u32 eid,iEventHandler*handler) {
    T_EventHandlersMap::iterator iter = subscribers_.find(eid);
    if (iter != subscribers_.end()) {
        T_EventHandlers::iterator hd_iter = std::find(iter->second.begin(), iter->second.end(), handler);
        if (hd_iter != iter->second.end()) {
            iter->second.erase(hd_iter);
        }
    }
}



void DispatchMsgService::handleAllResponseEvent(NetworkInterface* interface) {
    bool done = false;
    while (!done) {
        //从队列中中取事件(此处做同步互斥)
        iEvent* ev = nullptr;
        thread_mutex_lock(&queue_mutex);
        if (!response_events.empty()) {//响应事件队列不为空
            ev = response_events.front();
            //LOG_DEBUG("DispatchMsgService::handleAllResponseEvent - get Evqueue eid:%d\n", ev->get_eid());
            response_events.pop();
        }
        else {
            done = true;
        }
        thread_mutex_unlock(&queue_mutex);
        if(!done){
            if (ev->get_eid() == EEVENTID_GET_MOBILE_CODE_RSP) {
                MobileCodeRspEv* mcre = static_cast<MobileCodeRspEv*>(ev);
                LOG_DEBUG("DispatchMsgService::handleAllResponseEvent - eid = %d\n",ev->get_eid());
                //做序列话,
                ConnectSession* cs = (ConnectSession*)ev->get_args();
                cs->response = ev;
                cs->message_len = mcre->ByteSize();
                cs->write_buf = new char[cs->message_len + MESSAGE_HEADER_LEN];

                //组装头部
                memcpy(cs->write_buf, MESSAGE_HEADER_ID, 4);
                *((u16*)(cs->write_buf + 4)) = EEVENTID_GET_MOBILE_CODE_RSP;
                *((i32*)(cs->write_buf + 6)) = cs->message_len;
                mcre->SerializeToArray(cs->write_buf + MESSAGE_HEADER_LEN, cs->message_len);
                interface->send_response_message(cs);
                
            }
            else if (ev->get_eid() == EEVENTID_EXIT_RSP) {
                ConnectSession* cs = (ConnectSession*)ev->get_args();
                cs->response = ev;
                interface->send_response_message(cs);
                //将会话重置
               
            }
            else if (ev->get_eid() == EEVENTID_LOGIN_RSP) {
                LoginResEv* lgre = static_cast<LoginResEv*>(ev);
                LOG_DEBUG("DispatchMsgService::handleAllResponseEvent - eid = %d\n", ev->get_eid());
                ConnectSession* cs = (ConnectSession*)ev->get_args();
                cs->response = ev;
                cs->message_len = lgre->ByteSize();
                cs->write_buf = new char[cs->message_len + MESSAGE_HEADER_LEN];

                //组装头部
                memcpy(cs->write_buf, MESSAGE_HEADER_ID, 4);
                *((u16*)(cs->write_buf + 4)) = EEVENTID_LOGIN_RSP;
                *((i32*)(cs->write_buf + 6)) = cs->message_len;
                lgre->SerializeToArray(cs->write_buf + MESSAGE_HEADER_LEN, cs->message_len);
                interface->send_response_message(cs);
                
            }
        }
    }
}