#ifndef SHBK_COMMON_INICONFIG_H_
#define SHBK_COMMON_INICONFIG_H_

#include"configdef.h"
#include<string>
#include<iostream>
//using namespace std;

class Iniconfig{
    

protected:
    Iniconfig();
public:

    ~Iniconfig();
    bool loadfile(const std::string &path);//加载文件
    const st_env_config &getconfig();//获取配置
    static Iniconfig* getInstance();

private:
    st_env_config _config;
    bool _isloade;
    static Iniconfig* config;  //单例
};

#endif
