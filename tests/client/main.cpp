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

#include "clipsclient/clipsclient.h"

/* ** ********************************************************
* Global variables
* *** *******************************************************/

std::shared_ptr<ClipsClient> clientPtr;




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
void cleanup();
void messageReceivedHandler(const ClipsClientPtr& client, const std::string& msg);
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
	clientPtr = ClipsClient::create();
	printf("Running ClipsClient test\n");
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
	if(!clientPtr->connect(address, port)){
		fprintf(stderr, "Could not connect to CLIPS on %s:%u.\n", address.c_str(), port);
		fprintf(stderr, "Run the server and pass the right parameters.\n");
		std::exit(-1);
	}
	clientPtr->addMessageReceivedHandler(&messageReceivedHandler);
	printf("Connected to CLIPS server\n");
}


/**
 * Requests CLIPS to load the specified file
 * @param file Path to file to load
 */
void reqLoadFile(const std::string& file){
	printf("Press enter to load %s", file.c_str());
	while (std::cin.get() != '\n');
	clientPtr->clear();
	sleep_ms( 250 );
	clientPtr->loadFile(file);
}


/**
 * Requests CLIPS to reset the KDB
 */
void reqReset(){
	printf("Press enter to reset CLIPS");
	while (std::cin.get() != '\n');
	clientPtr->reset();
}


/**
 * Requests CLIPS to run the KDB engine
 */
void reqRun(){
	printf("Press enter to run CLIPS");
	while (std::cin.get() != '\n');
	clientPtr->run();
	sleep_ms(1000);
}


/**
 * Requests CLIPS to assert a fact.
 */
void reqAssert(const std::string& fact){
	printf("Press enter to assert %s", fact.c_str());
	while (std::cin.get() != '\n');
	clientPtr->assertFact( fact );
}


/**
 * Requests CLIPS to retract a fact.
 */
void reqRetract(const std::string& fact){
	printf("Press enter to retract %s", fact.c_str());
	while (std::cin.get() != '\n');
	clientPtr->retractFact( fact );
}


void cleanup(){
	clientPtr->disconnect();
}


void messageReceivedHandler(const ClipsClientPtr& client, const std::string& msg){
	printf( "CLIPS: %s%c", msg.c_str(), *(msg.end()) == '\n' ? 0 : *(msg.end()) );
}


/**
 * Sleeps the current execution thread for the specified amount of time
 * @param ms The amount of time in milliseconds
 */
static inline void sleep_ms(size_t ms){
	std::this_thread::sleep_for( std::chrono::milliseconds(ms) );
}
