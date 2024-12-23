#include "clipsclient.h"
#include <boost/bind/bind.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;


ClipsClient::ClipsClient(const Private&) :
	is(&buffer){}



ClipsClient::~ClipsClient(){
	disconnect();
}


ClipsClientPtr ClipsClient::getPtr(){
	return shared_from_this();
}


ClipsClientPtr ClipsClient::create(){
	return std::make_shared<ClipsClient>(Private());
}


bool ClipsClient::connect(const std::string& address, uint16_t port){
	if(socketPtr) return false;
	tcp::endpoint remote_endpoint{boost::asio::ip::address::from_string(address), port};
	socketPtr = std::make_shared<boost::asio::ip::tcp::socket>(io_service);
	try{
		socketPtr->connect(remote_endpoint);
	}
	catch(int ex){
		fprintf(stderr, "Could not connect to CLIPS on %s:%u.\n", address.c_str(), port);
		fprintf(stderr, "Run the server and pass the right parameters.\n");
		return false;
	}

	buffer.prepare(0xffff);
	beginReceive();
	serviceThreadPtr = std::shared_ptr<boost::thread>( new boost::thread(
		[this](){
				this->io_service.run();
			}
	));
	onConnected();
	return true;
}



void ClipsClient::disconnect(){
	if(serviceThreadPtr){
		io_service.stop();
		serviceThreadPtr->join();
		onDisconnected();
	}
	socketPtr = NULL;
}


void ClipsClient::loadFile(const std::string& file){
	sendRaw( "(load " + file + ")" );
}



void ClipsClient::reset(){
	sendRaw("(reset)");
}



void ClipsClient::clear(){
	sendRaw("(reset)");
}



void ClipsClient::run(int32_t n){
	if( n < -1 ) n = -1;
	sendRaw( "(run "+ std::to_string(n) +")" );
}



void ClipsClient::assertFact(const std::string& fact){
	sendRaw( "(assert " + fact + ")" );
}



void ClipsClient::retractFact(const std::string& fact){
	sendRaw( "(retract " + fact + ")" );
}


bool ClipsClient::send(const std::string& s){
	if(!socketPtr || !socketPtr->is_open() ) return false;
	socketPtr->send( asio::buffer(s) );
	return true;
}



bool ClipsClient::sendRaw(const std::string& s){
	if(!socketPtr || !socketPtr->is_open() ) return false;

	uint16_t packetsize = 7 + s.length();
	char buffer[packetsize];
	size_t i = 0;
	buffer[i++] = *((char*)&packetsize);
	buffer[i++] = *((char*)&packetsize +1);
	buffer[i++] = 0; buffer[i++] = 'r'; buffer[i++] = 'a'; buffer[i++] = 'w'; buffer[i++] = ' ';
	for(;i < packetsize; ++i)
		buffer[i] = s[i-7];
	socketPtr->send( asio::buffer(buffer, packetsize) );

	return true;
}



void ClipsClient::beginReceive(){
	asio::async_read(*socketPtr, buffer,
		asio::transfer_at_least(3),
		boost::bind(
			&ClipsClient::asyncReadHandler, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred)
		);
}



void ClipsClient::asyncReadHandler(const boost::system::error_code& error, size_t bytes_transferred){
	if(error){
		disconnect();
		return;
	}

	do{
		// 1. Read message header to read only complete messages.
		// If header is incomplete, the bytes read are returned to the buffer
		uint16_t msgsize;
		is.read((char*)&msgsize, sizeof(msgsize));
		if(buffer.size() < (msgsize - 2) ){
			is.unget(); is.unget();
			break;
		}
		// 2. Read the whole message. Bytes read are removed from the buffer by the istream
		std::string s(msgsize-=2, 0);
		is.read(&s[0], msgsize);
		// 3. Publish the read string.
		onMessageReceived(s);
		// Repeat while buffer has data
		}while(buffer.size() > 0);

		beginReceive();
}



void ClipsClient::onConnected(){
	int i = 1;
	for(auto it = connectedHandlers.begin(); it != connectedHandlers.end(); ++it){
		try{ (*it)( getPtr() ); }
		catch(int err){}
	}
}


void ClipsClient::onDisconnected(){
	for(auto it = disconnectedHandlers.begin(); it != disconnectedHandlers.end(); ++it){
		try{ (*it)( getPtr() ); }
		catch(int err){}
	}
}


void ClipsClient::onMessageReceived(const std::string& s){
	for(auto it = messageReceivedHandlers.begin(); it != messageReceivedHandlers.end(); ++it){
		try{ (*it)( getPtr(), s ); }
		catch(int err){}
	}
}


void ClipsClient::addConnectedHandler(std::function<void(const ClipsClientPtr&)> handler){
	if(!handler) return;
	connectedHandlers.push_back(handler);
}


void ClipsClient::addDisconnectedHandler(std::function<void(const ClipsClientPtr&)> handler){
	if(!handler) return;
	disconnectedHandlers.push_back(handler);
}


void ClipsClient::addMessageReceivedHandler(std::function<void(const ClipsClientPtr&, const std::string&)> handler){
	if(!handler) return;
	messageReceivedHandlers.push_back(handler);
}



void ClipsClient::removeConnectedHandler(std::function<void(const ClipsClientPtr&)> handler){
	if(!handler) return;

	typedef void(HT)(const ClipsClientPtr&);
	auto htarget = handler.target<HT>();
	for(auto it = connectedHandlers.begin(); it != connectedHandlers.end(); ++it){
		if (it->target<HT>() != htarget) continue;
		connectedHandlers.erase(it);
	}
}


void ClipsClient::removeDisconnectedHandler(std::function<void(const ClipsClientPtr&)> handler){
	if(!handler) return;

	typedef void(HT)(const ClipsClientPtr&);
	auto htarget = handler.target<HT>();
	for(auto it = disconnectedHandlers.begin(); it != disconnectedHandlers.end(); ++it){
		if (it->target<HT>() != htarget) continue;
		disconnectedHandlers.erase(it);
	}
}


void ClipsClient::removeMessageReceivedHandler(std::function<void(const ClipsClientPtr&, const std::string&)> handler){
	if(!handler) return;

	typedef void(HT)(const ClipsClientPtr&, const std::string&);
	auto htarget = handler.target<HT>();
	for(auto it = messageReceivedHandlers.begin(); it != messageReceivedHandlers.end(); ++it){
		if (it->target<HT>() != htarget) continue;
		messageReceivedHandlers.erase(it);
	}
}
