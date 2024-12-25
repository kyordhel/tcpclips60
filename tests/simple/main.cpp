/** @file main.cpp
* @author Mauricio Matamoros
*
* Anchor point (main function) for the test node
*
*/

/** @cond */
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
/** @endcond */

namespace asio = boost::asio;
using asio::ip::tcp;

/* ** ********************************************************
* Global variables
* *** *******************************************************/
/**
 * Service required for async communications
 */
asio::io_service io_service;

std::shared_ptr<boost::thread> serviceThreadPtr;

/**
 * Pointer to the socket object used to connect to the clips server
 */
std::shared_ptr<tcp::socket> socketPtr;

/**
 * Buffer to receive messages asynchronously
 */
asio::streambuf buffer;

/**
 * Stream used to read the buffer
 */
std::istream is(&buffer);


/* ** ********************************************************
* Prototypes
* *** *******************************************************/
int main(int argc, char **argv);
void connect(const std::string& address, uint16_t port);
void reqLoadFile(const std::string& file);
void reqReset();
void reqRun();
void reqAssert(const std::string& fact);
void reqRetract(const std::string& fact);
void publish(const std::string& s);
void cleanup();
void beginReceive();
void asyncReadHandler(const boost::system::error_code& error, size_t bytes_transferred);
static inline void sleep_ms(size_t ms);


/* ** ********************************************************
* Main (program anchor)
* *** *******************************************************/
/**
 * Program anchor
 * @param  argc The number of arguments to the program
 * @param  argv The arguments passed to the program
 * @return      The program exit code
 */
int main(int argc, char **argv){
	connect("127.0.0.1", 5000);

	reqLoadFile("cubes.clp");
	reqReset();
	reqRun();

	printf("\n\nCool, right!? Let's retry adding another cube...\n");
	reqReset();
	reqAssert("(block G)");
	reqAssert("(on-top-of (upper nothing)(lower G))");
	reqAssert("(on-top-of (upper G)(lower D))");
	reqRun();

	printf("\n\nNow with yet another cube...\n");
	reqReset();
	printf("\nUpon reset, we lost G, so let's add it again...\n");
	reqAssert("(block G)");
	reqAssert("(on-top-of (upper nothing)(lower G))");
	reqAssert("(on-top-of (upper G)(lower D))");
	printf("\nNow with H...\n");
	reqAssert("(block H)");
	reqAssert("(on-top-of (upper nothing)(lower H))");
	reqAssert("(on-top-of (upper H)(lower A))");
	reqRun();

	printf("All done. Shutting down... ");

	cleanup();
	printf("Bye!\n");
	return 0;
}


/* ** ********************************************************
* Function definitions
* *** *******************************************************/
void connect(const std::string& address, uint16_t port){
	tcp::endpoint remote_endpoint{boost::asio::ip::address::from_string(address), port};
	socketPtr = std::make_shared<boost::asio::ip::tcp::socket>(io_service);
	try{
		socketPtr->connect(remote_endpoint);
	}
	catch(int ex){
		fprintf(stderr, "Could not connect to CLIPS on %s:%u.\n", address.c_str(), port);
		fprintf(stderr, "Run the server and pass the right parameters.\n");
		std::exit(-1);
	}

	buffer.prepare(0xffff);
	beginReceive();
	serviceThreadPtr = std::shared_ptr<boost::thread>( new boost::thread(
		[](){
				io_service.run();
				serviceThreadPtr = NULL;
			}
	));
}


/**
 * Requests CLIPS to load the specified file
 * @param file Path to file to load
 */
void reqLoadFile(const std::string& file){
	printf("Press enter to load %s", file.c_str());
	while (std::cin.get() != '\n');
	publish( "(clear)" );
	sleep_ms( 250 );
	publish( "(load cubes.clp)" );
}


/**
 * Requests CLIPS to reset the KDB
 */
void reqReset(){
	printf("Press enter to reset CLIPS");
	while (std::cin.get() != '\n');
	publish( "(reset)" );
}


/**
 * Requests CLIPS to run the KDB engine
 */
void reqRun(){
	printf("Press enter to run CLIPS");
	while (std::cin.get() != '\n');
	publish( "(run -1)" );
	sleep_ms(1000);
}


/**
 * Requests CLIPS to assert a fact.
 */
void reqAssert(const std::string& fact){
	printf("Press enter to assert %s", fact.c_str());
	while (std::cin.get() != '\n');
	publish( "(assert " + fact + ")" );
}


/**
 * Requests CLIPS to retract a fact.
 */
void reqRetract(const std::string& fact){
	printf("Press enter to retract %s", fact.c_str());
	while (std::cin.get() != '\n');
	publish( "(retract " + fact + ")" );
}


/**
 * Sends the given strng to CLIPS
 * @param s The string to send
 */
void publish(const std::string& s){
	static int cmdId = 1;

	if(!socketPtr || !socketPtr->is_open() ) return;
	// 2byte size + 0x00 + 4byte cmdId + command + space + args
	uint16_t packetsize = 11 + s.length();
	char buffer[packetsize];
	std::memcpy(buffer, &packetsize, 2);
	buffer[2] = 0;
	std::memcpy(buffer+3, &cmdId, 4);
	std::memcpy(buffer+7, "raw ", 4);
	s.copy(buffer+11, s.length());

	socketPtr->send( asio::buffer(buffer, packetsize) );
	++cmdId;
}


void beginReceive(){
	asio::async_read(*socketPtr, buffer,
		asio::transfer_at_least(3),
		&asyncReadHandler);
}


void asyncReadHandler(const boost::system::error_code& error, size_t bytes_transferred){

	if(error){
		fprintf(stderr, "Disconnected from server.\nExiting.\n");
		exit(-1);
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
		// 3. Print read string.
		if( std::strlen(s.c_str()) > 0 )
			printf( "CLIPS: %s%c", s.c_str(), *(s.end()) == '\n' ? 0 : *(s.end()) );
		// Repeat while buffer has data
		}while(buffer.size() > 0);

		beginReceive();
}


// void subCallback(const std_msgs::String::ConstPtr& msg){
// }

void cleanup(){
	io_service.stop();
	if(serviceThreadPtr)
		serviceThreadPtr->join();
}


/**
 * Sleeps the current execution thread for the specified amount of time
 * @param ms The amount of time in milliseconds
 */
static inline void sleep_ms(size_t ms){
	std::this_thread::sleep_for( std::chrono::milliseconds(ms) );
}
