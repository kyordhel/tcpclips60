#include "reply.h"

namespace asio = boost::asio;
using asio::ip::tcp;

Reply::Reply(uint32_t cmdId, bool success, const std::string& result):
	cmdId(cmdId), success(success), result(result){}


uint32_t Reply::getCommandId() const{
	return cmdId;
}


bool Reply::getSuccess() const{
	return success;
}



std::string Reply::getResult() const{
	return result;
}



bool Reply::matches(const Request& r){
	return r.getCommandId() == cmdId;
}



bool Reply::matches(const RequestPtr& r){
	if(!r) return false;
	return r->getCommandId() == cmdId;
}



bool Reply::matches(const Reply& rep, const Request& req){
	return rep.getCommandId() == req.getCommandId();

}



bool Reply::matches(const ReplyPtr& rep, const RequestPtr& req){
	if(!rep || !req) return false;
	return rep->getCommandId() == req->getCommandId();
}



ReplyPtr Reply::fromMessage(const std::string& message){
	// Reply is: 0x00 + 4byte CmdId + 1byte success flag + Response (if any).
	if( message.length() < 6) return NULL;
	if( message[0] ) return NULL;

	uint32_t cmdId;
	message.copy((char*)&cmdId,   4, 1);

	bool success = message[5];
	std::string result = (message.length() > 6) ? message.substr(6) : "";
	return ReplyPtr(new Reply(cmdId, success, result));
}
