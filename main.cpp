#include"bike.pb.h"
#include<iostream>
#include"ievent.h"
#include"events_def.h"
#include"user_event_handler.h"
#include"DispatchMsgService.h"
#include"NetworkInterface.h"
#include<unistd.h>
#include"iniconfig.h"
#include"Logger.h"
#include"sqlconnection.h"
#include"SqlTables.h"
#include"BusProcessor.h"


using namespace std;
using namespace tutorial;

int main(int argc,char**argv) {

   

    if (argc != 3) {
        printf("Please input shbk <conifg file path><log file config>!\n");
        return -1;
    }
    if (!Logger::instance()->init(std::string(argv[2]))) {
        fprintf(stderr, "init log moudle failed.\n");
        return -2;
    }

    Iniconfig *config=Iniconfig::getInstance();
    if (!config->loadfile(std::string(argv[1]))) {
        // printf("load %s failed.\n",argv[1]);
        LOG_ERROR("load %s failed.", argv[1]);
        return -3;
    }

    st_env_config conf_args = config->getconfig();
    LOG_INFO("[database] ip:%s port: %d user: %s pwd: %s db: %s [server] port: %d\n", \
        conf_args.db_ip.c_str(), conf_args.db_port, conf_args.db_user.c_str(), \
        conf_args.db_pwd.c_str(), conf_args.db_name.c_str(), conf_args.svr_port);
    
    
   //MySqlConnection mysqlConn;

    std::shared_ptr<MySqlConnection> mysqlconn(new MySqlConnection);
    if (!mysqlconn->Init(conf_args.db_ip.c_str(), conf_args.db_port,
        conf_args.db_user.c_str(), conf_args.db_pwd.c_str(), conf_args.db_name.c_str())){
        
        LOG_ERROR("Database init failed. exit!\n");
        return -4;
    }
    BussinessProcessor busPro(mysqlconn);
    busPro.init();
   // userEventHandler ueh;
 
    DispatchMsgService *DMS = DispatchMsgService::getInstance();
    DMS->open();
    ////产生一个事件
    //MobileCodeReqEv* mcrv = new MobileCodeReqEv("15668336138");

    NetworkInterface* NIF = new NetworkInterface();

    NIF->start(conf_args.svr_port);
    while (1 == 1) {
        NIF->netowrk_event_dispatch();
        LOG_DEBUG("netowrk_event_dispatch...\n");
        sleep(1);
    }
    DMS->close();//线程池销毁
    sleep(5);
    return 0;
}