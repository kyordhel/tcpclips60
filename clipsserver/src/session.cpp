#include "server.h"
#include "session.h"
#include <boost/bind/bind.hpp>

namespace ph = std::placeholders;
namespace asio = boost::asio;
using asio::ip::tcp;


Session::Session(std::shared_ptr<boost::asio::ip::tcp::socket> socketPtr,
				 Server& server):
	socketPtr(socketPtr), server(server){
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
	buffer.prepare(0xffff);
	// asio::async_read_until(*socketPtr, buffer, "\n",
	asio::async_read(*socketPtr, buffer,
		// boost::bind(&Session::checkTransferComplete, this,
			// boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred),
		boost::asio::transfer_at_least(3),
		boost::bind(&Session::asyncReadHandler, this,
			boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
	);
}

size_t Session::checkTransferComplete(const boost::system::error_code& error, size_t bytes_transferred){
	// uint16_t* hdrPtr = (uint16_t*)buffer.data();
	// printf("HDR: 0x%4x\tBT: 0x%4x\n", *hdrPtr, bytes_transferred);
	return 0;
}

void Session::asyncReadHandler(const boost::system::error_code& error, size_t bytes_transferred){
	if(error){
		server.removeSession(endpoint);
		// delete this;
		return;
	}

	std::istream is(&buffer);

	do{
		// 1. Fetch header.
		// Header is 2 bytes and contains the size of the message
		uint16_t msgsize = peekMessageHeader(is);
		// 2. If the buffer size is smaller than the header the message is incomplete. Skip.
		if(buffer.size() < msgsize) break;
		// 3. Retrieve the message
		std::string s = fetchStringFromBuffer(is);
		// 4. Enqueue the message
		server.enqueueTcpMessage( TcpMessage::makeShared(endpoint, s) );
	}while(buffer.size() > 0);
	beginAsyncReceivePoll();
}

uint16_t Session::peekMessageHeader(std::istream& is){
	uint16_t hdr;
	is.read((char*)&hdr, sizeof(hdr));
	is.unget(); is.unget();
	return hdr;
}

std::string Session::fetchStringFromBuffer(std::istream& is){
	// Reads header again
	uint16_t bytesToRead;
	is.read((char*)&bytesToRead, sizeof(bytesToRead));
	bytesToRead-= sizeof(bytesToRead);

	std::string s(bytesToRead+1, 0);
	is.read(&s[0], bytesToRead);
	return s;
}


void Session::send(const std::string& s){
	if(!this->socketPtr || !this->socketPtr->is_open() ) return;
	// asio::write( *socketPtr, asio::buffer(message) );
	//
	uint16_t packetsize = 2 + s.length();
	char buffer[packetsize];
	buffer[0] = *((char*)&packetsize);
	buffer[1] = *((char*)&packetsize +1);
	for(size_t i = 0; i < s.length(); ++i)
		buffer[i+2] = s[i];
	// buffer[packetsize-1] = 0;
	asio::write(*socketPtr, asio::buffer(buffer, packetsize));
}


std::shared_ptr<Session> Session::makeShared(
			std::shared_ptr<tcp::socket> socketPtr,
			Server& server
	){
	return std::shared_ptr<Session>(new Session(socketPtr, server));
}
