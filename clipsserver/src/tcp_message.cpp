#include "tcp_message.h"

TcpMessage::TcpMessage(const std::string& source, const std::string& message):
	source(source), message(message){}

std::string& TcpMessage::getSource(){
	return source;
}

std::string& TcpMessage::getMessage(){
	return message;
}

std::shared_ptr<TcpMessage> TcpMessage::makeShared(const std::string& source, const std::string& message){
	return std::shared_ptr<TcpMessage>(new TcpMessage(source, message));
}