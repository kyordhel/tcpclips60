#include "request.h"

namespace asio = boost::asio;
using asio::ip::tcp;

uint32_t Request::lastCommandId = 0;

Request::Request(){}

Request::Request(const std::string& command, const std::string& args) :
	cmdId(++Request::lastCommandId), cmd(command), args(args){}



uint32_t Request::getCommandId() const{
	return cmdId;
}



std::string Request::getCommand() const{
	return cmd;
}



std::string Request::getArgs() const{
	return args;
}



std::vector<char> Request::getPayload() const{
	std::string content = cmd;
	if( !args.empty() ) content += " " + args;

	std::vector<char> payload;
	uint16_t packetsize = 7 + content.length();
	payload.resize(packetsize, 0);
	char* buffer = payload.data();
	std::memcpy(buffer, &packetsize, 2);
	buffer[2] = 0;
	std::memcpy(buffer+3, &cmdId, 4);
	content.copy(buffer+7, content.length());
	return payload;
}



RequestPtr Request::fromMessage(const std::string& message){
	if( message[0] || ( message.length() < 6) ) return NULL;

	size_t argsPos = message.find( ' ' , 5);
	if(argsPos == 5) return NULL;

	RequestPtr rqPtr = RequestPtr( new Request() );
	rqPtr->cmd = message.substr(5, argsPos);
	rqPtr->args = (argsPos != std::string::npos) ? message.substr(argsPos+1) : "";
	message.copy( (char*)&(rqPtr->cmdId), 4, 1 );
	return rqPtr;
}



RequestPtr Request::make_shared(const std::string& command, const std::string& args){
	return RequestPtr(new Request(command, args));
}
