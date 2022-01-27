#ifndef NS_IEVNET_HANDLER_H_
#define NS_IEVNET_HANDLER_H_
#include"ievent.h"
#include"eventtypes.h"

class iEventHandler {//事件处理基类
public:
	virtual iEvent* handle(const iEvent* ev) { return NULL; }
	virtual ~iEventHandler() {}
    iEventHandler(const char *name):name_(name) {}
	std::string& getname() { return name_; }
private:
	std::string name_;
};


#endif