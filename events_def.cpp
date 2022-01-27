#include<iostream>
#include<sstream>
#include"events_def.h"

std::ostream& MobileCodeReqEv::dump(std::ostream &out)const {
	out << "MobileCodeReq sn = " << get_sn()<<",";
	out << "mobile = " << _msg.mobile()<<endl;
	return out;
}

std::ostream& MobileCodeRspEv::dump(ostream& out)const {
	out << "MobileCodeRspEv sn = " << get_sn() << "\t";
	out << "code = " << msg_.code() << "\t";
	out << "describle = " << msg_.data() << "\t";
	out << "icode = " << msg_.icode() << endl;

	return out;
}

std::ostream& LoginReqEv::dump(std::ostream& out) const
{
	out << "LoginReqEv sn = " << get_sn() << ",";
	out << "mobile = " << msg_.mobile() << std::endl;
	out << "icode = " << msg_.icode() << std::endl;
	return out;
}

std::ostream& LoginResEv::dump(std::ostream& out) const
{
	out << "LoginResEv sn =" << get_sn() << ",";
	out << "code = " << msg_.code() << std::endl;
	out << "describe = " << msg_.desc() << std::endl;
	return out;
}
