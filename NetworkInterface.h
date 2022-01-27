#ifndef BRK_INTERFACE_NETWORK_INTERFACE_H_
#define BRK_INTERFACE_NETWORK_INTERFACE_H_
#include<event.h>
#include<event2/event.h>
#include<event2/listener.h>
#include<string>
#include"ievent.h"


#define MESSAGE_HEADER_LEN 10
#define MESSAGE_HEADER_ID "FBEB"

enum class SESSION_STATUS {
    SS_REQUEST,
	SS_RESPONSE
};

enum class MESSAGE_STATUS {
	MS_READ_HEADER = 0,
	MS_READ_MESSAGE = 1,   //��Ϣ����δ��ʼ
	MS_READ_DONE = 2,      //��Ϣ�������
	MS_SENDIN = 3,         //��Ϣ������

};

typedef struct _ConnectSession {
	char remote_ip[32];   //�ͻ���ip��ַ

	SESSION_STATUS session_stat;      //��ǰ���������������������������Ӧ

	iEvent* request;                  //����ͻ��˷����������¼�
	MESSAGE_STATUS req_stat;
	//�����״̬
	iEvent* response;                 //����
	MESSAGE_STATUS res_stat;

	u16 eid;                           //���浱ǰ�����¼���ID
	i32 fd;                            //���浱ǰ���͵��ļ����

	struct bufferevent* bev;            
	u32 message_len;                     //��ǰ��д��Ϣ�ĳ���
	u32 read_message_len;                //�Ѿ���д����Ϣ����
	u32 sent_len;                        //�ѷ�����Ϣ�ĳ���
	char* read_buf;                      //�������Ϣ�Ļ�����
	char header[MESSAGE_HEADER_LEN + 1]; //����ͷ��
	char* write_buf;
	u32 read_header_len;                 //�Ѷ�ȡ��ͷ������
}ConnectSession;

class NetworkInterface {

public:
	NetworkInterface();
	~NetworkInterface();

	bool start(int port);
	void close();
	
	static void listener_cb(struct evconnlistener* listener, evutil_socket_t fd,
	struct sockaddr* sock, int socklen, void* arg);
	static void handle_request(struct bufferevent* bev, void* arg);//������ص�
	static void handle_response(struct bufferevent* bev, void* arg);//��������ص�
	static void handle_error(struct bufferevent* bev, short event, void* arg);
	void netowrk_event_dispatch(); //���ϵĵ�������������������¼�
	void send_response_message(ConnectSession* cs);
private:
	struct evconnlistener *listener_;
	struct event_base* base_;
};


#endif