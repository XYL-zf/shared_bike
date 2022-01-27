//#include<iostream>
#include<string>
//#include<string.h>
#include"iniconfig.h"
#include"iniparser/iniparser.h"

//using namespace std;
Iniconfig::Iniconfig():_isloade(false){//默认构造函数
}


Iniconfig::~Iniconfig(){//析构函数
}

Iniconfig* Iniconfig::config = nullptr;

Iniconfig* Iniconfig::getInstance() {

    if (!config) {
        config = new Iniconfig();
    }
    return config;
}

bool Iniconfig::loadfile(const std::string &path){
    dictionary  *   ini = NULL;
    ini = iniparser_load(path.c_str());
    if (ini==NULL) {
        fprintf(stderr, "cannot parse file: %s\n", path.c_str());
        return false;
    }
     /*
     [database]
     ip       = 127.0.0.1 ;
     port     = 3306 ;
     user     = root ;
     pwd      = 123456 ;
     db       = qiniubike;
     [server]
     port     = 9090;
     */
     //成功加载配置文件
    const char *ip = iniparser_getstring(ini,"database:ip","127.0.0.1");
    int port = iniparser_getint(ini,"database:port",3306);
    const char * user = iniparser_getstring(ini,"database:user","root");
    const char * pwd  = iniparser_getstring(ini,"database:pwd","12345");
    const char *db = iniparser_getstring(ini,"database:db","qiniubike");
    int sport = iniparser_getint(ini,"server:port",9090);
    
    _config = st_env_config(std::string(ip),port,std::string(user),\
std::string(pwd),std::string(db),sport);    

    iniparser_freedict(ini);
    _isloade = true;
    return _isloade;
}


    const st_env_config &Iniconfig::getconfig(){
        return _config;
}



