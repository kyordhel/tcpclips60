#include "clipsclient.h"
#include "request.h"
#include "reply.h"

#include <regex>
#include <boost/bind/bind.hpp>


namespace asio = boost::asio;
using asio::ip::tcp;


ClipsClient::ClipsClient(const Private&) :
	is(&buffer), clipsStatus(NULL){}



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
	catch(...){
		// fprintf(stderr, "Could not connect to CLIPS on %s:%u.\n", address.c_str(), port);
		// fprintf(stderr, "Run the server and pass the right parameters.\n");
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
	abortAllRPC();
	if(serviceThreadPtr){
		io_service.stop();
		serviceThreadPtr->join();
		onDisconnected();
	}
	socketPtr = NULL;
}


void ClipsClient::loadFile(const std::string& file){
	// sendRaw( "(load " + file + ")" );
	rpc("load", file);
}



void ClipsClient::reset(){
	// sendRaw("(reset)");
	rpc("reset");
}



void ClipsClient::clear(){
	// sendRaw("(clear)");
	rpc("clear");
}



void ClipsClient::run(int32_t n){
	if( n < -1 ) n = -1;
	// sendRaw( "(run "+ std::to_string(n) +")" );
	rpc("run", std::to_string(n));
}



void ClipsClient::assertFact(const std::string& fact){
	rpc("assert", fact);
}



void ClipsClient::retractFact(const std::string& fact){
	rpc("raw", "(retract " + fact + ")" );
}

	/**
	 * Requests ClipsServer to execute a command.
	 * A command is any of
	 * 		assert   Asserts the fact given in args
	 * 		raw      Injects the string in CLIPS language contained in args
	 * 		path     Sets the working path of CLIPSServer
	 * 		load     Loads the CLP or DAT file specidied in args
	 * 		log      Sets the log level of CLIPSServer
	 *
	 * @param  cmd  The command to execute
	 * @param  args The command to execute
	 * @return      true if the command was successfully executed, false otherwise
	 */
bool ClipsClient::execute(const std::string& cmd, const std::string& args){
	static std::regex rxInt("-?\\d{1,9}");
	static std::regex rxPrint("facts|rules|agenda");
	static std::regex rxWatch("functions|globals|facts|rules");
	std::smatch match;

	if(cmd == "reset") return rpc(cmd);
	else if(cmd == "clear") return rpc(cmd);
	else if(cmd == "run") {
		if( args.empty() ) return rpc(cmd, "-1");
		if( std::regex_match(args, match, rxInt) ) return rpc(cmd, args);
		return false;
	}
	else if(cmd == "print")  return std::regex_match(args, match, rxPrint) ? rpc(cmd, args) : false;
	else if(cmd == "watch")  return std::regex_match(args, match, rxWatch) ? rpc(cmd, args) : false;
	else if(cmd == "assert") return !args.empty() ? rpc(cmd, args) : false;
	else if(cmd == "raw")    return !args.empty() ? rpc(cmd, args) : false;
	else if(cmd == "path")   return !args.empty() ? rpc(cmd, args) : false;
	else if(cmd == "load")   return !args.empty() ? rpc(cmd, args) : false;
	else if(cmd == "log")    return !args.empty() ? rpc(cmd, args) : false;
	return false;
}


bool ClipsClient::query(const std::string& query, std::string& result){
	return rpc("query", query, result);
}


uint32_t ClipsClient::getWatches(){
	rpc("watch");
	return clipsStatus ? clipsStatus->getWatches() : -1;
}


uint32_t ClipsClient::toggleWatch(const std::string& watch){
	if( (watch == "functions") || (watch == "globals") || (watch == "facts") || (watch == "rules") )
		rpc("watch", watch);
	return clipsStatus ? clipsStatus->getWatches() : -1;
}


bool ClipsClient::send(const std::string& s){
	if(!socketPtr || !socketPtr->is_open() ) return false;
	socketPtr->send( asio::buffer(s) );
	return true;
}


bool ClipsClient::sendCommand(const std::string& command, const std::string& args, uint32_t& cmdId){
	static uint32_t __cmdId = 1;
	if(!socketPtr || !socketPtr->is_open() ) return false;

	Request rq(command, args);
	cmdId = rq.getCommandId();
	socketPtr->send( asio::buffer(rq.getPayload()) );

	return true;
}


bool ClipsClient::awaitResponse(int cmdId, bool& success, std::string& result){
	std::unique_lock<std::mutex> lock(pcmutex);
	bool aborted = false;
	// do{
	// 	pccv.wait(lock);
	// } while(!hasReponseArrived(cmdId, aborted));
	pccv.wait(lock, [&]{ return hasReponseArrived(cmdId, aborted); } );
	if(!pendingCommands[cmdId] || aborted) return false;
	result = pendingCommands[cmdId]->getResult();
	pendingCommands.erase(cmdId);
	return true;
}


bool ClipsClient::hasReponseArrived(uint32_t cmdId, bool& aborted){
	if( !pendingCommands.count(cmdId) ){
		aborted = true;
		return true;
	}
	return pendingCommands[cmdId] == NULL;
}


bool ClipsClient::rpc(const std::string& cmd, const std::string& args, std::string& result){
	uint32_t cmdId = 0;
	bool success = false;
	if( !sendCommand(cmd, args, cmdId) ) return false;
	{std::lock_guard<std::mutex> lock(pcmutex);
		pendingCommands[cmdId] = NULL;
	}
	if( !awaitResponse(cmdId, success, result) ) return false;
	return success;
}

bool ClipsClient::rpc(const std::string& cmd){
	std::string result;
	return rpc(cmd, "", result);
}

bool ClipsClient::rpc(const std::string& cmd, const std::string& args){
	std::string result;
	return rpc(cmd, args, result);
}


void ClipsClient::abortAllRPC(){
	std::unique_lock<std::mutex> lock(pcmutex);
	pendingCommands.clear();
	lock.unlock();
	pccv.notify_all();
}


bool ClipsClient::sendRaw(const std::string& s){
	uint32_t cmdId;
	return sendCommand("raw", s, cmdId);
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
		// If message size is 2 or less (empty/malformed), discard.
		if(!(msgsize-=2)) continue;
		// 2. Read the whole message. Bytes read are removed from the buffer by the istream
		std::string s(msgsize, 0);
		is.read(&s[0], msgsize);

		// 3. If the message is a command's response, process it. Else publish the read string.
		if(s[0] == 0) handleResponseMesage(s);
		else onMessageReceived(s);
		// Repeat while buffer has data
	}while(buffer.size() > 0);

	beginReceive();
}


void ClipsClient::handleResponseMesage(const std::string& s){
	ReplyPtr rplptr = Reply::fromMessage(s);
	if( rplptr ){
		if(rplptr->getCommandId() == -1){
			updateStatus(rplptr);
			return;
		}
		std::unique_lock<std::mutex> lock(pcmutex);
		if( !pendingCommands.count(rplptr->getCommandId()) )  return;
		pendingCommands[rplptr->getCommandId()] = rplptr;
		lock.unlock();
		pccv.notify_all();
	}
}



void ClipsClient::updateStatus(ReplyPtr r){
	const std::string& result = r->getResult();
	if( (r->getCommandId() != -1) || !r->getSuccess() || (result.substr(0, 9) != "watching:" ))
		return;
	// clipsStatus = std::stoi( result.substr(9) );
	ClipsStatusPtr ncs = ClipsStatus::fromString(result);
	if(!ncs) return;
	clipsStatus = ncs;
	onClipsStatusChanged();
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


void ClipsClient::onClipsStatusChanged(){
	for(auto it = clipsStatusChangedHandlers.begin(); it != clipsStatusChangedHandlers.end(); ++it){
		try{ (*it)( getPtr(), clipsStatus ); }
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

void ClipsClient::addClipsStatusChangedHandler(std::function<void(const ClipsClientPtr&, const ClipsStatusPtr&)> handler){
	if(!handler) return;
	clipsStatusChangedHandlers.push_back(handler);
}


#if __GNUC__ > 10

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


void ClipsClient::removeClipsStatusChangedHandler(std::function<void(const ClipsClientPtr&, const ClipsStatusPtr&)> handler){
	if(!handler) return;

	typedef void(HT)(const ClipsClientPtr&, const ClipsStatusPtr&);
	auto htarget = handler.target<HT>();
	for(auto it = clipsStatusChangedHandlers.begin(); it != clipsStatusChangedHandlers.end(); ++it){
		if (it->target<HT>() != htarget) continue;
		clipsStatusChangedHandlers.erase(it);
	}
}

#endif
