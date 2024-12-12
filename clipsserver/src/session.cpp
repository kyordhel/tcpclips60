#include "session.h"
#include <boost/bind/bind.hpp>

namespace ph = std::placeholders;
namespace asio = boost::asio;
using asio::ip::tcp;


Session::Session(std::shared_ptr<boost::asio::ip::tcp::socket> socketPtr,
				 sync_queue<std::string>& queue):
	socketPtr(socketPtr), queue(queue){
		std::ostringstream os;
		auto ep = socketPtr->remote_endpoint();
		os << ep;
		endpoint = os.str();
		beginAsyncReceivePoll();
	}

Session::~Session(){
	if(this->socketPtr)
		this->socketPtr->close();
	this->socketPtr = NULL;
}

std::string Session::getEndPointStr() const{
	return endpoint;
}

std::shared_ptr<boost::asio::ip::tcp::socket> Session::getSocketPtr() const{
	return socketPtr;
}


void Session::beginAsyncReceivePoll(){
	asio::async_read_until(*socketPtr, buffer, "\n",
	// asio::async_read(*socketPtr, buffer,
		boost::bind(&Session::asyncReadHandler, this,
			boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
	);
}

void Session::asyncReadHandler(const boost::system::error_code& error, size_t bytes_transferred){
	if(error){
		delete this;
		return;
	}

	// asio::streambuf::const_buffers_type cbt = buffer.data();
	// std::string s(	boost::asio::buffers_begin(cbt),
	// 				boost::asio::buffers_begin(cbt) + bytes_transferred);
	std::istream is(&buffer);
	std::string s;
	std::getline(is, s);
	printf("[%s]: %s\n", endpoint.c_str(), s.c_str());
	queue.produce(s);
	beginAsyncReceivePoll();
}

void Session::send(const std::string& s){
	if(!this->socketPtr || !this->socketPtr->is_open() ) return;
	// asio::write( *socketPtr, asio::buffer(message) );
	socketPtr->send( asio::buffer(s) );
}

std::shared_ptr<Session> Session::makeShared(
			std::shared_ptr<tcp::socket> socketPtr,
			sync_queue<std::string>& queue
	){
	return std::shared_ptr<Session>(new Session(socketPtr, queue));
}
