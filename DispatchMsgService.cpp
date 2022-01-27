
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
    //�����¼�
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
        //���յ������¼����������ӣ��ȴ��̳߳���Ӧ���ͻ���
        iEvent *rsp = dmg->process(ev);
       
        if (rsp) {
            rsp->dump(std::cout);
            rsp->set_args(ev->get_args());
        }
        else {
            //������ֹ��Ӧ�¼�
            rsp = new ExitRspEv();
            rsp->set_args(ev->get_args());
        }
        //��rsp��ӣ�ע�⣺rsp���ǰһ��Ҫ��������Ϊ�ж���̻߳�ͬʱ�����������
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
    
    //�����¼��Ƿ񱻶�����
    T_EventHandlersMap::iterator handlers = subscribers_.find(eid);
    if (handlers == subscribers_.end()) {
        LOG_WARN("DispatchMsgService:no event handler subscribe %d\n", eid);
        return NULL;
    }
    //���¼��б�����, ���Ҷ��ĸ��¼��Ĵ�����
    iEvent* rsp = NULL;
    for (auto iter = handlers->second.begin(); iter != handlers->second.end(); iter++) {
        iEventHandler* handler = *iter;       //*iter�Ǽ̳���iEventHandler�࣬�ø���ָ����ͳһ�Դ���
        LOG_DEBUG("DispatchMsgService:get handler name:%s\n", handler->getname().c_str());
        rsp = handler->handle(ev);
        //��ͬһ���¼��ж��������,�Ǿͽ������ķ������͸ĳ�vector<iEvent*>����
        //ÿ��һ�������ߴ�����һ���¼����ͽ��䴦��������vector<iEvnet*>��
        //��󷵻������������¶���
    }
    
    return rsp;
}


i32 DispatchMsgService::enqueue(iEvent* ev) {
    if (ev == NULL)return -1;
   // LOG_DEBUG("DispatchMsgService::enqueue - ev will be in queue... eid = %d\n", ev->get_eid());
    //����һ������
    thread_task_t* task = thread_task_alloc(0);
    task->ctx = ev;
    task->handler = DispatchMsgService::svc;

    //Ͷ������
    return thread_task_post(tp, task);
}


//�����¼���ָ�����¼����ĸ��¼�����������
void DispatchMsgService::subscribe(u32 eid, iEventHandler* handler) {
    //һ���¼����԰󶨶���¼�������
    //�����¼����������Ȳ�ѯһ�¸��¼���û��֮ǰ�ͱ��������������ģ����
    LOG_DEBUG("DispatchMsgService::subscribe eid:%u\n", eid);
    T_EventHandlersMap::iterator iter = subscribers_.find(eid);//�����������Ƿ��и��¼�id
    if (iter != subscribers_.end()) {//���¼��Ѿ�����
        //�������Ͳ��Ҹô������Ƿ��ٸ������У����������ھͽ��ô�������ӽ�ȥ��
        T_EventHandlers::iterator hd_iter = std::find(iter->second.begin(), iter->second.end(), handler);
        if (hd_iter == iter->second.end()) {//��������û��handler������
            iter->second.push_back(handler);//ֱ�ӽ�handler��ӵ�����
        }
    }
    else {
        //���¼�������
        subscribers_[eid].push_back(handler);    //�����¼�����Ӧ�Ĵ������󶨼���
    }
}


//�˶�
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
        //�Ӷ�������ȡ�¼�(�˴���ͬ������)
        iEvent* ev = nullptr;
        thread_mutex_lock(&queue_mutex);
        if (!response_events.empty()) {//��Ӧ�¼����в�Ϊ��
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
                //�����л�,
                ConnectSession* cs = (ConnectSession*)ev->get_args();
                cs->response = ev;
                cs->message_len = mcre->ByteSize();
                cs->write_buf = new char[cs->message_len + MESSAGE_HEADER_LEN];

                //��װͷ��
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
                //���Ự����
               
            }
            else if (ev->get_eid() == EEVENTID_LOGIN_RSP) {
                LoginResEv* lgre = static_cast<LoginResEv*>(ev);
                LOG_DEBUG("DispatchMsgService::handleAllResponseEvent - eid = %d\n", ev->get_eid());
                ConnectSession* cs = (ConnectSession*)ev->get_args();
                cs->response = ev;
                cs->message_len = lgre->ByteSize();
                cs->write_buf = new char[cs->message_len + MESSAGE_HEADER_LEN];

                //��װͷ��
                memcpy(cs->write_buf, MESSAGE_HEADER_ID, 4);
                *((u16*)(cs->write_buf + 4)) = EEVENTID_LOGIN_RSP;
                *((i32*)(cs->write_buf + 6)) = cs->message_len;
                lgre->SerializeToArray(cs->write_buf + MESSAGE_HEADER_LEN, cs->message_len);
                interface->send_response_message(cs);
                
            }
        }
    }
}