#include "server.h"

#include <regex>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>

#include <boost/bind/bind.hpp>

#include <unistd.h>

#include "utils.h"
#include "clipswrapper.h"

namespace ph = std::placeholders;
namespace asio = boost::asio;
using asio::ip::tcp;

/* ** ********************************************************
* Macros
* *** *******************************************************/
#define contains(s1,s2) s1.find(s2) != std::string::npos


/* ** ********************************************************
* Local helpers
* *** *******************************************************/
static inline
bool ends_with(const std::string& s, const std::string& end){
	if (end.size() > s.size()) return false;
	return std::equal(end.rbegin(), end.rend(), s.rbegin());
}

static inline
void split_path(const std::string& fpath, std::string& dir, std::string& fname){
	size_t slashp = fpath.rfind("/");
	if(slashp == std::string::npos){
		dir = std::string();
		fname = fpath;
		return;
	}
	dir = fpath.substr(0, slashp);
	fname = fpath.substr(slashp+1);
}

static inline
std::string get_current_path(){
	char buff[FILENAME_MAX];
	getcwd(buff, sizeof(buff));
	return std::string(buff);
}




/* ** ********************************************************
* Constructor
* *** *******************************************************/
Server::Server():
	// clipsFile("cubes.dat"),
	port(5000), defaultMsgInFact("network 0.0.0.0:0"), acceptorPtr(NULL),
	flgFacts(false), flgRules(false), clppath(get_current_path()){
}

Server::~Server(){
	stop();
}


/* ** ********************************************************
*
* Class methods
* Initialization
*
* *** *******************************************************/
bool Server::init(int argc, char **argv){
	if( !parseArgs(argc, argv) ) return false;

	if( !initTcpServer() ) return false;
	// std::this_thread::sleep_for(std::chrono::milliseconds(delay));

	initCLIPS(argc, argv);
	// publishStatus();

	return true;
}


void Server::initCLIPS(int argc, char **argv){
	clips::initialize();
	clips::rerouteStdin(argc, argv);
	clips::clear();
	std::cout << "Clips ready" << std::endl;

	// Load clp files specified in file
	loadFile(clipsFile);
	if(flgFacts) clips::toggleWatch(clips::WatchItem::Facts);
	if(flgRules) clips::toggleWatch(clips::WatchItem::Rules);

	// Further CLIPS initialization (routers, etc).
	clips::QueryRouter& qr = clips::QueryRouter::getInstance();
	qr.addLogicalName("wdisplay"); // Capture display info
	qr.addLogicalName("wtrace");   // Capture trace info
	qr.addLogicalName("stdout");   // Capture everything else
}


bool Server::initTcpServer(){

	// tcp::endpoint listen_ep{tcp::v4(), port};
	tcp::endpoint listen_ep{{}, port};
	acceptorPtr = std::shared_ptr<tcp::acceptor>(new tcp::acceptor(io_context, listen_ep));
	acceptorPtr->set_option(tcp::acceptor::reuse_address(true));
	// acceptor.async_accept(boost::bind(&Server::acceptHandler, this));
	// auto socketPtr = std::make_shared(tcp::socket);
	// acceptor.async_accept([this](const boost::system::error_code& error, tcp::socket peer)->void{ acceptHandler(error, peer); });
	// tcp::socket socket = acceptor.accept();

	std::shared_ptr<tcp::socket> socketPtr(new tcp::socket(io_context));
	// std::shared_ptr<Session> sessionPtr(new Session(io_context));
	acceptorPtr->async_accept(
		*socketPtr,
		// *sessionPtr,
		boost::bind(&Server::acceptHandler, this, boost::asio::placeholders::error, socketPtr));
	// acceptorPtr->bind(listen_ep);
	acceptorPtr->listen();
	printf("Listening on port %u\n", port);
	return true;
}


void Server::acceptHandler(const boost::system::error_code& error, std::shared_ptr<tcp::socket> socketPtr){
	if(!error){
		auto sp = Session::makeShared(socketPtr, *this);
		clients[sp->getEndPointStr()] = sp;
		printf("Connected client %s\n", sp->getEndPointStr().c_str());
	}

	std::shared_ptr<tcp::socket> nextSckt(new tcp::socket(io_context));
	acceptorPtr->async_accept(
		*nextSckt,
		boost::bind(&Server::acceptHandler, this, boost::asio::placeholders::error, nextSckt));
}


/* ** ********************************************************
*
* Class methods: Clips wrappers
*
* *** *******************************************************/
void Server::assertFact(const std::string& s, const std::string& fact, bool resetFactListChanged) {
	std::string f = fact.empty() ? defaultMsgInFact : fact;
	std::string as = "(" + fact + " " + s + ")";
	clips::assertString( as );
	if(resetFactListChanged)
		clips::setFactListChanged(0);
	printf("Asserted string %s\n", as.c_str());
}


void Server::clearCLIPS(){
	clips::clear();
	printf("KDB cleared (clear)\n");
}


void Server::resetCLIPS(){
	clips::reset();
	printf("KDB reset (reset)\n");
}


void Server::sendCommand(std::string const& s){
	printf("Executing command: %s\n", s.c_str());
	clips::sendCommand(s);
}


bool Server::loadClp(const std::string& fpath){
	printf("Loading file '%s'...\n", fpath.c_str() );
	if( !clips::load( fpath ) ){
		printf("Error in file '%s' or does not exist", fpath.c_str());
		return false;
	}
	printf("File %s loaded successfully\n", fpath.c_str());
	return true;
}


bool Server::loadDat(const std::string& fpath){
	if( fpath.empty() ) return false;
	std::ifstream fs;
	fs.open(fpath);

	if( fs.fail() || !fs.is_open() ){
		fprintf(stderr, "File '%s' does not exists\n", fpath.c_str());
		return false;
	}

	bool err = false;
	std::string line, fdir, fname;
	std::string here = get_current_path();
	split_path(fpath, fdir, fname);
	if(!fdir.empty()) chdir(fdir.c_str());
	printf("Loading '%s'...\n", fname.c_str());
	while(!err && std::getline(fs, line) ){
		if(line.empty()) continue;
		// size_t slashp = fpath.rfind("/");
		// if(slashp != std::string::npos) line = fdir + line;
		if (!loadClp(line)) err = true;
	}
	fs.close();
	chdir(here.c_str());
	printf(err? "Aborted.\n" : "Done.");

	return !err;
}


bool Server::loadFile(std::string const& fpath){
	printf("Current path '%s'\n", get_current_path().c_str() );
	if(ends_with(fpath, ".dat"))
		return loadDat(fpath);
	else if(ends_with(fpath, ".clp"))
		return loadClp(fpath);
	return false;
}


void Server::enqueueTcpMessage(std::shared_ptr<TcpMessage> messagePtr){
	queue.produce(messagePtr);
}

/**
 * Parses messages from network clients
 * Re-implements original parse_network_message by Jesús Savage
 * @param msgPtr A pointer to the received message
 * @param m      The received message

 */
void Server::parseMessage(std::shared_ptr<TcpMessage> msg){
	// printf("[%s]: %s\n", msg->getSource().c_str(), msg->getMessage().c_str());

	std::string& m = msg->getMessage();
	if((m[0] == 0) && (m.length()>1)){
		handleCommand(m.substr(1));
		return;
	}

	std::string& ep = msg->getSource();
	assertFact(m, "network " + ep);
}


static inline
void splitCommand(const std::string& s, std::string& cmd, std::string& arg){
	std::string::size_type sp = s.find(" ");
	if(sp == std::string::npos){
		cmd = s;
		arg.clear();
	}
	else{
		cmd = s.substr(0, sp);
		arg = s.substr(sp+1);
	}
}


void Server::handleCommand(const std::string& c){
	std::string cmd, arg;
	splitCommand(c, cmd, arg);

	// printf("Received command %s", c.c_str());
	if(cmd == "assert") { clips::assertString(arg); }
	else if(cmd == "reset") { resetCLIPS(); }
	else if(cmd == "clear") { clearCLIPS(); }
	else if(cmd == "raw")   { sendCommand(arg); }
	else if(cmd == "path")  { handlePath(arg); }
	else if(cmd == "print") { handlePrint(arg); }
	else if(cmd == "watch") { handleWatch(arg); }
	else if(cmd == "load")  { loadFile(arg); }
	else if(cmd == "run")   { handleRun(arg); }
	else if(cmd == "log")   { handleLog(arg); }
	else return;

	// ROS_INFO("Handled command %s", c.c_str());
}


void Server::handleLog(const std::string& arg){
}


void Server::handlePath(const std::string& path){
	if(chdir(path.c_str()) != 0){
		fprintf(stderr, "Can't access {%s}: %s\n", path.c_str(), std::strerror(errno));
		printf("Reset clppath  to {%s}\n", clppath.c_str() );
	}
	clppath = path;
}


void Server::handlePrint(const std::string& arg){
	if(arg == "facts"){       clips::printFacts();  }
	else if(arg == "rules"){  clips::printRules();  }
	else if(arg == "agenda"){ clips::printAgenda(); }
}


void Server::handleRun(const std::string& arg){
	int n = std::stoi(arg);
	clips::run(n);
}


void Server::handleWatch(const std::string& arg){
	if(arg == "functions"){    clips::toggleWatch(clips::WatchItem::Deffunctions); }
	else if(arg == "globals"){ clips::toggleWatch(clips::WatchItem::Globals);      }
	else if(arg == "facts"){   clips::toggleWatch(clips::WatchItem::Facts);        }
	else if(arg == "rules"){   clips::toggleWatch(clips::WatchItem::Rules);        }
	publishStatus();
}



/* ** ********************************************************
*
* Class methods: ROS-related
*
* *** *******************************************************/
bool Server::broadcast(const std::string& message){
	for(auto it = clients.begin(); it != clients.end(); ++it)
		it->second->send( message );
	return true;
}


bool Server::sendTo(const std::string& cliEP, const std::string& message){
	if (clients.find(cliEP) == clients.end()){
		fprintf(stderr, "Client %s disconnected or does not exist", cliEP.c_str());
		return false;
	}
	clients[cliEP]->send( message );
	return true;
}


bool Server::publishStatus(){
	std::string status("watching:" + std::to_string((int)clips::getWatches()));
	return broadcast(status);
}


/* ** ********************************************************
*
* Class methods: Multithreaded execution
*
* *** *******************************************************/
void Server::stop(){
	running = false;
	if(asyncThread.joinable())
		asyncThread.join();
}


void Server::runAsync(){
	asyncThread = std::thread(&Server::run, this);
}


void Server::run(){
	if(running) return;
	running = true;
	// Loop forever
	while(running){
		io_context.poll();
		if( queue.empty() ){
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			continue;
		}
		parseMessage( queue.consume() );
	}
}



/* ** ********************************************************
*
* Class methods: Misc
*
* *** *******************************************************/
bool Server::parseArgs(int argc, char **argv){
	std::string pname(argv[0]);
	pname = pname.substr(pname.find_last_of("/") + 1);
	// Read input parameters
	if (argc <= 1) {
		printDefaultArgs(pname);
		return true;
	}

	for(int i = 1; i < argc; ++i){
		if (!strcmp(argv[i], "-h") || (i+1 >= argc) ){
			printHelp( pname );
			return false;
		}
		else if (!strcmp(argv[i],"-d")){
			clppath = std::string(argv[++i]);

			if(chdir(argv[i]) != 0){
				fprintf(stderr, "Can't access {%s}: %s\n", argv[i], std::strerror(errno));
				printf("Reset clppath  to {%s}\n", get_current_path().c_str() );
			}
		}
		else if (!strcmp(argv[i],"-e")){
			clipsFile = std::string(argv[++i]);
		}
		else if (!strcmp(argv[i],"-w")){
			flgFacts = atoi(argv[++i]);
		}
		else if (!strcmp(argv[i],"-r")){
			flgRules = atoi(argv[++i]);
		}
		else if (!strcmp(argv[i],"-p")){
			port = std::stoi(argv[++i]);
		}

	}
	return true;
}



void Server::printDefaultArgs(std::string const& pname){
	std::cout << "Using default parameters:" << std::endl;
	std::cout << "    "   << pname;
	std::cout << " -p "   << port;
	std::cout << " -d "   << clppath;
	std::cout << " -e "   << ( (clipsFile.length() > 0) ? clipsFile : "''");
	std::cout << " -w "   << flgFacts;
	std::cout << " -r "   << flgRules;
	std::cout << std::endl << std::endl;
}



void Server::printHelp(std::string const& pname){
	std::cout << "Usage:" << std::endl;
	std::cout << "    " << pname << " ";
	std::cout << "-p port ";
	std::cout << "-d clp base path (where clips files are)";
	std::cout << "-e clipsFile ";
	std::cout << "-w watch_facts ";
	std::cout << "-r watch_rules ";
	std::cout << std::endl << std::endl;
	std::cout << "Example:" << std::endl;
	std::cout << "    " << pname << " -e virbot.dat -w 1 -r 1"  << std::endl;
}

// bool Bridge::srvQueryKDB(rosclips::QueryKDB::Request& req, rosclips::QueryKDB::Response& res){
// 	clips::query(req.query, res.result);
// 	return true;
// }
