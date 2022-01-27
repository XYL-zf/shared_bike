#include "NetworkInterface.h"
#include"DispatchMsgService.h"
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>

//�мǣ�ConnectSession ������C���͵ĳ�Ա����
static ConnectSession* session_init(int fd, struct bufferevent* bev) {

    if (bev == NULL) {
        LOG_DEBUG("session_init:bev is null\n");
        return NULL;
    }
    ConnectSession* temp = new ConnectSession();
    if (!temp) {
        LOG_DEBUG("malloc failed...\n");
        return NULL;
    }
    memset(temp, '\0', sizeof(ConnectSession));
    temp->bev = bev;
    temp->fd = fd;

    return temp;
}

void session_free(ConnectSession* cs) {
    if (cs) {
        if (cs->read_buf) {
            delete[]cs->read_buf;
            cs->read_buf = NULL;
        }

        if (cs->write_buf) {
            delete[]cs->write_buf;
            cs->read_buf = NULL;
        }
        delete cs;
    }
}
NetworkInterface::NetworkInterface()
{
    base_ = nullptr;
    listener_ = nullptr;
}

NetworkInterface::~NetworkInterface()
{
    close();
}

bool NetworkInterface::start(int port)
{
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    
    base_ = event_base_new();
    listener_= evconnlistener_new_bind(base_,NetworkInterface::listener_cb, base_,
        LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
        512, (struct sockaddr*)&sin,
        sizeof(struct sockaddr_in));
}

void session_reset(ConnectSession* cs) {
    if (cs) {
        if (cs->read_buf) {
            delete[]cs->read_buf;
            cs->read_buf = nullptr;
        }
        if (cs->write_buf) {
            delete[]cs->write_buf;
            cs->write_buf = nullptr;
        }

        cs->message_len = 0;
        cs->read_message_len = 0;
        cs->read_header_len = 0;
        //�޸ĻỰ״̬
        cs->session_stat = SESSION_STATUS::SS_REQUEST;//��������
        cs->req_stat = MESSAGE_STATUS::MS_READ_HEADER; //�ö�ȡͷ����
    }
}
void NetworkInterface::close() {
    if (base_) {
        event_base_free(base_);
        base_ = nullptr;
    }
    if (listener_) {
        evconnlistener_free(listener_);
        listener_ = nullptr;
    }
}

void NetworkInterface::listener_cb(evconnlistener* listener, evutil_socket_t fd,
    sockaddr* sock, int socklen, void* arg)
{
    LOG_DEBUG("accept a client:%d\t", fd);
   struct event_base* base = (struct event_base*)arg;
   struct bufferevent* bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
   ConnectSession* cs = session_init(fd, bev);
   cs->session_stat = SESSION_STATUS::SS_REQUEST;
   cs->req_stat = MESSAGE_STATUS::MS_READ_HEADER;
   //����Զ��IP��ַ

   strcpy(cs->remote_ip, inet_ntoa(((sockaddr_in*)sock)->sin_addr));
   LOG_DEBUG("remote ip:%s\n", cs->remote_ip);

   //���ûص�
   bufferevent_setcb(bev,handle_request, handle_response, handle_error,cs);
   bufferevent_enable(bev,EV_READ | EV_PERSIST);
   //���ó�ʱ����
   bufferevent_settimeout(bev, 10, 10);//��Щ�β��������������ļ��л���ӵú���
}


/*********************************
* Ӧ���ڲ㴫��Э��
           4���ֽ�       2���ֽ�          4���ֽ�
 �����ʽ   FBEB         �¼�ID           ���ݳ���    ��������
    
    1.����ʶ : ��ͷ���������ʶ��������ʶ���Ŀ�ʼ
    2.�¼����� : �¼�ID���̶������ֽڱ�ʾ
    3.���ݳ��� : ���ݰ��Ĵ�С���̶�����4�ֽڡ�
    4.�������� : �������ݣ�����Ϊ����ͷ����ĳ��ȴ�С��
***********************************/
   
void NetworkInterface::handle_request(bufferevent* bev, void* arg)
{
    ConnectSession* cs = (ConnectSession*)arg;
    //����Ҫ���Ự״̬
    if (cs->session_stat != SESSION_STATUS::SS_REQUEST) {
        LOG_DEBUG("NetworkInterface::handle_request: - wrong session state[%d]\n", cs->session_stat);
        return;
    }

    LOG_ERROR("NetworkInterface::handle_request: - req_stat :%d", cs->req_stat);
    //��ȡͷ��
    if (cs->req_stat == MESSAGE_STATUS::MS_READ_HEADER) {//��ȡ״̬���ڶ�ȡͷ����״̬
        i32 len = bufferevent_read(bev, cs->header + cs->read_header_len,
            MESSAGE_HEADER_LEN - cs->read_header_len);
        cs->read_header_len += len;
        cs->header[cs->read_header_len] = '\0';
        LOG_DEBUG("recv form cilent<<<<%s header len : %d\n", cs->header, cs->read_header_len);
        //�ж�ͷ�����Ѿ���ȡ��10���ֽ�
        //���û�ж�ȡ���Ǹ��ֽ��ǲ�����������ȡ��Ϣ�����ݵģ�ֻ�еȵ��´η�������
        //�����¼�ʱ��������������10�ֽ��Ժ��ڽ�������Ĵ�����
        if (cs->read_header_len == MESSAGE_HEADER_LEN) {
            //������ȡ����ͷ��,��� ���ı�ʶ  MESSAGE_HEADER_ID "FBEB"
            if (strncmp(cs->header, MESSAGE_HEADER_ID, strlen(MESSAGE_HEADER_ID))== 0 ) {
                //��ȡ�¼�ID
                cs->eid = *((u16*)(cs->header + 4));     //��ȡ�ͻ��˷������¼��¼�id
                /*�ͻ����������������ݵĳ���
                ������ݳ���Ҫ�ں���ķ�Χ�ڣ���Ϊ���̫�󣬷����������ڴ����ᵼ��崻�
                */
                LOG_DEBUG("NetworkInterface::handle_request - eid = %d\n", cs->eid);
                cs->message_len = *((i32*)(cs->header + 6));
                LOG_DEBUG("NetworkInterface::handle_request: - read %d bytes in header message len:%d\n",
                    cs->read_header_len, cs->message_len);

                if (cs->message_len<1 || cs->message_len>MAX_MESSAGE_LEN) { 
                    LOG_ERROR("NetworkInterface::handle_request - wrong message, len:%d\n", cs->message_len);
                    bufferevent_free(bev);
                    session_free(cs);
                    return;
                }

                cs->read_buf = new char[MAX_MESSAGE_LEN];
                //��ȡ����
                cs->req_stat = MESSAGE_STATUS::MS_READ_MESSAGE;
                cs->read_message_len = 0;
            }
            else {//��Ч������
                LOG_ERROR("NetworkInterface::handle_request - Invalid request form %s\n", cs->remote_ip);
                bufferevent_free(bev);
                session_free(cs);
                return;
            }
        }
    }
    //��ȡ״̬���ڶ�ȡ���� ����״̬��ͬʱbuffer�������ݿ��Զ�
    if (cs->req_stat == MESSAGE_STATUS::MS_READ_MESSAGE && evbuffer_get_length(bufferevent_get_input(bev)) > 0) {
        i32 len = bufferevent_read(bev, cs->read_buf + cs->read_message_len,
            cs->message_len - cs->read_message_len);
        cs->read_message_len += len;
        LOG_DEBUG("NetworkInterface::handle_request - buffervent_read %d bytes,message len:%d\n",
            cs->read_message_len, cs->message_len);

        //�����ȡ��Ͼͽ����¼�
        if (cs->read_message_len == cs->message_len) {
            //�Ự״̬��Ϊ��Ӧ
            cs->session_stat = SESSION_STATUS::SS_RESPONSE;
            //�����¼�
           iEvent *ev = DispatchMsgService::getInstance()->parseEvent(cs->read_buf,
               cs->read_message_len, cs->eid);

           //�����¼���read_buf�ͷţ�
           delete[] cs->read_buf;
           cs->read_buf = nullptr;
           cs->read_message_len = 0;

           if (ev) {//���¼�Ͷ�ݵ��̳߳����������
               ev->set_args(cs);
               DispatchMsgService::getInstance()->enqueue(ev);
           }
           else {//�¼�Ϊ��
               LOG_ERROR("NetworkInterface::handle_request - ev is null,remote ip:%s,eid:%d\n",
                   cs->remote_ip,cs->eid);
               bufferevent_free(bev);
               session_free(cs);
               return;
           }
        }
    }
}

void NetworkInterface::handle_response(bufferevent* bev, void* arg)
{
    LOG_DEBUG("NetworkInterface::handle_response...");
}

//��ʱ ���ӹر� ��д������쳣ָ���ûص�����
void NetworkInterface::handle_error(bufferevent* bev, short event, void* arg)
{
    LOG_DEBUG("NetworkInterface::handle_error...");
    //���ӹر�
    if (event & BEV_EVENT_EOF) {
        LOG_DEBUG("NetworkInterface::handle_error:connection close...\n");
    }
    else if ((event&BEV_EVENT_TIMEOUT)&&(event&BEV_EVENT_READING)) {//����ʱ
        LOG_WARN("NetworkInterface::handle_error:read timeout...\n");
    }
    else if ((event & BEV_EVENT_TIMEOUT) && (event & BEV_EVENT_WRITING)) {
        LOG_WARN("NetworkInterface::handle_error:write timeout...\n");
    }
    else if (event & BEV_EVENT_ERROR) {
        LOG_ERROR("NetworkInterface::handle_error:other error...\n");
    }

    //�ͷ���Դ
    ConnectSession* cs = (ConnectSession*)arg;
    bufferevent_free(bev);
    session_free(cs);
}

void NetworkInterface::netowrk_event_dispatch()
{
    event_base_loop(base_, EVLOOP_NONBLOCK);
    //������Ӧ�¼����ظ���Ӧ��Ϣ δ�����...
    DispatchMsgService::getInstance()->handleAllResponseEvent(this);

}

void NetworkInterface::send_response_message(ConnectSession* cs)
{
    if (cs->response == nullptr) {
        LOG_DEBUG("NetworkInterface::send_response_message - cs->response is null\n");
        bufferevent_free(cs->bev);
        if (cs->request) {
            delete[] cs->request;
        }
        session_free(cs);
    }
    else {
        //LOG_DEBUG("NetworkInterface::send_response_message -send message to client...\n");
        bufferevent_write(cs->bev, cs->write_buf, cs->message_len + MESSAGE_HEADER_LEN);
        session_reset(cs);

    }
}

