#include "NetworkInterface.h"
#include"DispatchMsgService.h"
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>

//切记：ConnectSession 必须是C类型的成员变量
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
        //修改会话状态
        cs->session_stat = SESSION_STATUS::SS_REQUEST;//接收请求
        cs->req_stat = MESSAGE_STATUS::MS_READ_HEADER; //该读取头部了
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
   //保存远端IP地址

   strcpy(cs->remote_ip, inet_ntoa(((sockaddr_in*)sock)->sin_addr));
   LOG_DEBUG("remote ip:%s\n", cs->remote_ip);

   //设置回调
   bufferevent_setcb(bev,handle_request, handle_response, handle_error,cs);
   bufferevent_enable(bev,EV_READ | EV_PERSIST);
   //设置超时处理
   bufferevent_settimeout(bev, 10, 10);//这些参参数设置在配置文件中会更加得合理
}


/*********************************
* 应用内层传输协议
           4个字节       2个字节          4个字节
 请求格式   FBEB         事件ID           数据长度    数据内容
    
    1.包标识 : 包头部的特殊标识，用来标识包的开始
    2.事件类型 : 事件ID，固定两个字节表示
    3.数据长度 : 数据包的大小，固定长度4字节。
    4.数据内容 : 数据内容，长度为数据头定义的长度大小。
***********************************/
   
void NetworkInterface::handle_request(bufferevent* bev, void* arg)
{
    ConnectSession* cs = (ConnectSession*)arg;
    //首先要检查会话状态
    if (cs->session_stat != SESSION_STATUS::SS_REQUEST) {
        LOG_DEBUG("NetworkInterface::handle_request: - wrong session state[%d]\n", cs->session_stat);
        return;
    }

    LOG_ERROR("NetworkInterface::handle_request: - req_stat :%d", cs->req_stat);
    //读取头部
    if (cs->req_stat == MESSAGE_STATUS::MS_READ_HEADER) {//读取状态处于读取头部的状态
        i32 len = bufferevent_read(bev, cs->header + cs->read_header_len,
            MESSAGE_HEADER_LEN - cs->read_header_len);
        cs->read_header_len += len;
        cs->header[cs->read_header_len] = '\0';
        LOG_DEBUG("recv form cilent<<<<%s header len : %d\n", cs->header, cs->read_header_len);
        //判断头部是已经读取了10个字节
        //如果没有读取到是个字节是不会进到下面读取消息的内容的，只有等到下次发来数据
        //触发事件时，继续读，读到10字节以后在进到下面的代码中
        if (cs->read_header_len == MESSAGE_HEADER_LEN) {
            //解析读取到的头部,检查 包的标识  MESSAGE_HEADER_ID "FBEB"
            if (strncmp(cs->header, MESSAGE_HEADER_ID, strlen(MESSAGE_HEADER_ID))== 0 ) {
                //获取事件ID
                cs->eid = *((u16*)(cs->header + 4));     //获取客户端发来的事件事件id
                /*客户端请求发送数据内容的长度
                这个数据长度要在合理的范围内，因为如果太大，服务器分配内存过多会导致宕机
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
                //读取数据
                cs->req_stat = MESSAGE_STATUS::MS_READ_MESSAGE;
                cs->read_message_len = 0;
            }
            else {//无效的请求
                LOG_ERROR("NetworkInterface::handle_request - Invalid request form %s\n", cs->remote_ip);
                bufferevent_free(bev);
                session_free(cs);
                return;
            }
        }
    }
    //读取状态处于读取数据 内容状态，同时buffer又有数据可以读
    if (cs->req_stat == MESSAGE_STATUS::MS_READ_MESSAGE && evbuffer_get_length(bufferevent_get_input(bev)) > 0) {
        i32 len = bufferevent_read(bev, cs->read_buf + cs->read_message_len,
            cs->message_len - cs->read_message_len);
        cs->read_message_len += len;
        LOG_DEBUG("NetworkInterface::handle_request - buffervent_read %d bytes,message len:%d\n",
            cs->read_message_len, cs->message_len);

        //如果读取完毕就解析事件
        if (cs->read_message_len == cs->message_len) {
            //会话状态改为回应
            cs->session_stat = SESSION_STATUS::SS_RESPONSE;
            //解析事件
           iEvent *ev = DispatchMsgService::getInstance()->parseEvent(cs->read_buf,
               cs->read_message_len, cs->eid);

           //生成事件后将read_buf释放，
           delete[] cs->read_buf;
           cs->read_buf = nullptr;
           cs->read_message_len = 0;

           if (ev) {//将事件投递到线程池任务队列中
               ev->set_args(cs);
               DispatchMsgService::getInstance()->enqueue(ev);
           }
           else {//事件为空
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

//超时 连接关闭 读写出错等异常指定得回调函数
void NetworkInterface::handle_error(bufferevent* bev, short event, void* arg)
{
    LOG_DEBUG("NetworkInterface::handle_error...");
    //连接关闭
    if (event & BEV_EVENT_EOF) {
        LOG_DEBUG("NetworkInterface::handle_error:connection close...\n");
    }
    else if ((event&BEV_EVENT_TIMEOUT)&&(event&BEV_EVENT_READING)) {//读超时
        LOG_WARN("NetworkInterface::handle_error:read timeout...\n");
    }
    else if ((event & BEV_EVENT_TIMEOUT) && (event & BEV_EVENT_WRITING)) {
        LOG_WARN("NetworkInterface::handle_error:write timeout...\n");
    }
    else if (event & BEV_EVENT_ERROR) {
        LOG_ERROR("NetworkInterface::handle_error:other error...\n");
    }

    //释放资源
    ConnectSession* cs = (ConnectSession*)arg;
    bufferevent_free(bev);
    session_free(cs);
}

void NetworkInterface::netowrk_event_dispatch()
{
    event_base_loop(base_, EVLOOP_NONBLOCK);
    //处理响应事件，回复响应消息 未完待续...
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

