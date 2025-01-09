/* ** *****************************************************************
* main.cpp
*
* Author: Mauricio Matamoros
*
* ** *****************************************************************/
/** @file clipscontrol/main.cpp
 * Anchor point (main function) for the clipscontrol client
 */


/** @cond */
#include <regex>
#include <string>
#include <thread>
#include <iostream>

#include <boost/asio.hpp>

#include "ncurseswin.h"
/** @endcond */

#include "clipsclient/clipsclient.h"

namespace asio = boost::asio;
using asio::ip::tcp;
using namespace clipscontrol;

/* ** ********************************************************
* Global variables
* *** *******************************************************/

/**
 * CLIPSServer default address
 */
std::string address     = "127.0.0.1";

/**
 * CLIPSServer default port
 */
uint16_t port           = 5000;

/**
 * Client object used to connect to CLIPS
 */
std::shared_ptr<ClipsClient> clientPtr;

/**
 * GUI window
 */
std::unique_ptr<NCursesWin> ncwPtr;

/**
 * Pointer to the [re]connection thread, if any.
 */
std::unique_ptr<std::thread> acThreadPtr;

/**
 * Indicates whether the application is running or shutting down
 */
bool running;


/* ** ********************************************************
* Prototypes
* *** *******************************************************/
void sendCommandToCLIPS(const std::string& cmd, const std::string& args);
void asyncConnectTask();
// void clstatSubsCallback(const std_msgs::String::ConstPtr& msg, NCursesWin* ncw);
bool queryHandler(const std::string& query, std::string& result);
void connectedEH(const ClipsClientPtr& client);
void disconnectedEH(const ClipsClientPtr& client);
void messageReceivedEH(const ClipsClientPtr&, const std::string& msg);
void clipsStatusChangedEH(const ClipsClientPtr&, const ClipsStatusPtr& status);

/* ** ********************************************************
* Main (program anchor)
* *** *******************************************************/
/**
 * Program anchor
 * @param  argc The number of arguments to the program
 * @param  argv The arguments passed to the program
 * @return      The program exit code
 */
int main(int argc, char** argv){
	running = true;
	NCursesWin ncw; // Creates and initializes gui
	ncwPtr = std::unique_ptr<NCursesWin>(&ncw);

	clientPtr = ClipsClient::create();
	clientPtr->addConnectedHandler(&connectedEH);
	clientPtr->addDisconnectedHandler(&disconnectedEH);
	clientPtr->addMessageReceivedHandler(&messageReceivedEH);
	clientPtr->addClipsStatusChangedHandler(&clipsStatusChangedEH);

	commandfunc cmdHandler(sendCommandToCLIPS);
	ncw.addCommandHandler(cmdHandler);
	queryfunc qf(queryHandler);
	ncw.addQueryHandler(qf);

	acThreadPtr = std::make_unique<std::thread>(asyncConnectTask);
	ncw.poll();
	running = false;
	clientPtr->disconnect();
	ncw.exitPoll();
	printf("Done.\n\n");
	return 0;
}


/* ** ********************************************************
* Function definitions
* *** *******************************************************/
void asyncConnectTask(){
	stderr = fopen("/dev/null", "w");
	while( running && !clientPtr->connect(address, port) )
		std::this_thread::sleep_for(std::chrono::seconds(1));
}

/**
 * Callback for the client's Message Received event
 * It will print the message in the main window of the GUI.
 * @param msg    The received message
 */
void messageReceivedEH(const ClipsClientPtr& client, const std::string& msg){
	if(msg.back() != '\n') ncwPtr->print(msg + "\n");
	else ncwPtr->print(msg);
}

/**
 * Callback for the client's Connected event.
 * It will update the GUI's CLIPS Online status
 * @param client The ClipsClient that raises the event
 */
void connectedEH(const ClipsClientPtr& client){
	ncwPtr->setCLIPSStatus(NCursesWin::CLIPSStatus::Online);
}


/**
 * Callback for the client's Disconnected event.
 * It will update the GUI's CLIPS Online status
 * @param client The ClipsClient that raises the event
 */
void disconnectedEH(const ClipsClientPtr& client){
	ncwPtr->setCLIPSStatus(NCursesWin::CLIPSStatus::Offline);
	if(running && !acThreadPtr)
		acThreadPtr = std::make_unique<std::thread>(asyncConnectTask);
}


/**
 * Callback for the client's ClipsStatusChanged event
 * It will update the GUI's watch status
 * @param client The ClipsClient that raises the event
 * @param status The clips status
 */
void clipsStatusChangedEH(const ClipsClientPtr&, const ClipsStatusPtr& status){
	if(status)
		ncwPtr->setWatchFlags(status->getWatches());
}

/**
 * Sends the given strng to CLIPS
 * @param s The string to send
 */
void sendCommandToCLIPS(const std::string& cmd, const std::string& args){
	clientPtr->execute(cmd, args);
}


bool queryHandler(const std::string& query, std::string& result){
	return clientPtr->query(query, result);
}
