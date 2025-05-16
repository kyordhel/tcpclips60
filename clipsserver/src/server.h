/* ** *****************************************************************
* clips_bridge.cpp
*
* Author: Mauricio Matamoros
*
* ** *****************************************************************/
/** @file clips_bridge.h
 * Definition of the Server class: a base class for a bridge
 * between ROS and CLIPS.
 */

#ifndef __SERVER_H__
#define __SERVER_H__
#pragma once

/** @cond */
#include <thread>
#include <string>
#include <iomanip>
#include <unordered_map>

#include <boost/asio.hpp>
/** @endcond */

#include "session.h"
#include "tcp_message.h"
#include "sync_queue.h"


/**
 * Implements a base class for a bridge between ROS and CLIPS
 */
class Server{
public:
	/**
	 * Initializes a new instance of Server
	 */
	Server();
	~Server();

	// Disable copy constructor and assignment op.
private:
	/**
	 * Copy constructor disabled
	 */
	Server(Server const& obj)        = delete;
	/**
	 * Copy assignment operator disabled
	 */
	Server& operator=(Server const&) = delete;


public:
	/**
	 * Initializes the bridge. Must be called after
	 * @param  argc  main's argc
	 * @param  argv  main's argv
	 * @return       true if initialization completed successfully, false otherwise
	 */
	bool init(int argc, char **argv);

	/**
	 * Loads a file
	 * @remark       Works only with clp or dat file extensions.
	 *               A dat file contains several clp files.
	 * @param  fpath The path of the file to load
	 * @return       true if the file was loaded successfully, false otherwise
	 */
	bool loadFile(std::string const& fpath);

	/**
	 * Loads a clp file
	 * @param  fpath The path of the file to load
	 * @return       true if the file was loaded successfully, false otherwise
	 */
	bool loadClp(std::string const& fpath);

	/**
	 * Loads a dat file
	 * @param  fpath The path of the file to load
	 * @return       true if the file was loaded successfully, false otherwise
	 */
	bool loadDat(std::string const& fpath);

	/**
	 * Runs the bridge, blocking the calling thread until ROS is shutdown
	 */
	void run();

	/**
	 * Runs the bridge in a separate thread, returning immediatelly
	 */
	void runAsync();

	/**
	 * Stops the bridge, regardless of whether it is running synchronously or asynchronously
	 */
	void stop();

	/**
	 * Enqueues a received TCP message in the server's message queue
	 * @param messagePtr A pointer to the received message
	 */
	void enqueueTcpMessage(std::shared_ptr<TcpMessage> messagePtr);

	/**
	 * Removes a session from the server. Called by Session upon disconnection.
	 * @param sPtr The remote endpoint of the session to remove.
	 */
	void removeSession(const std::string& srep);



protected:
	/**
	 * Asserts the input string as a fact as (assert (network s))
	 * @param s                    The string to be asserted
	 * @param fact                 Optional. The fact under which \p s
	 *                             will be asserted. When empty it
	 *                             defaults to whatever defaultMsgInFact
	 *                             is set. Default: an empty string.
	 * @param resetFactListChanged Optional. When true
	 *                             clips::setFactListChanged(0) is reset,
	 *                             telling clips that no changes to the
	 *                             facts list have been done.
	 *                             Default: true
	 */
	void assertFact(const std::string& s, const std::string& fact = "", bool resetFactListChanged = true);

	/**
	 * Injects a command into CLIPS by calling clisp::sendCommand(s)
	 * @param s The CLIPS command to be injected
	 */
	bool sendCommand(std::string const& s);

	/**
	 * Clears CLIPS by calling clips::clear()
	 */
	void clearCLIPS();
	/**
	 * Resets CLIPS by calling clips::reset()
	 */
	void resetCLIPS();
	// std::string& eval(std::string const& s); // Unsupported in 6.0


	/**
	 * Initializes CLIPS.
	 * It calls clips::initialize(), clips::rerouteStdin(argc, argv)
	 * and clips::clear() in that order before loading the file
	 * specified by Server::clips_file.
	 * @param argc The main's argc
	 * @param argv The main's argv
	 */
	virtual void initCLIPS(int argc, char **argv);

	/**
	 * Initializes the TCP server.
	 */
	virtual bool initTcpServer();

private:

	/**
	 * Parses messages received via network.
	 * Two types of messages are accepted: facts and commands.
	 * Any non-command string is considered a fact and thus is asserted
	 * with Server::assertFact().
	 * Commands are strings that start with a NULL character ('\\0').
	 * The following commands are supported:
	 * assert      calls clips::assertString()
	 * reset       calls clips::reset()
	 * clear       calls clips::clear()
	 * raw         Injects a code via sendCommand()
	 * print what  Prints facts, rules or agenda
	 * watch what  Toggles the specified watches
	 * load  file  Loads the specified file
	 * run num     Performs the specified number of runs
	 * log         Unimplemented
	 *
	 * @param cliEp      The message source. A string representation of the
	 *                   remote endpoint of the network client that sends the message
	 * @param message    The received message
	 */
	void parseMessage(std::shared_ptr<TcpMessage> m);
	// void parseMessage(const TcpMessage& m);

	/**
	 * Acknowledges reception/excecution of a message
	 * @param message   The message to acknowledge
	 * @param success   Optional. Indicates whether the command contained in the message
	 *                  was successfully executed. Default: true.
	 * @param response  Optional. Contains the execution result of the command contained
	 *                  in the message, if any. Default: an empty string.
	 */
	void acknowledgeMessage(std::shared_ptr<TcpMessage> message, bool success=true, const std::string& result = "");

	/**
	 * Handles commands received via topicIn
	 * @param c The received command message
	 */
	bool handleCommand(const std::string& c, std::string& result);

	/**
	 * Unimplemented
	 * @param arg Unimplemented
	 */
	bool handleLog(const std::string& arg);

	/**
	 * Handles path request commands received via topicIn
	 * @param path The path where CLP files are
	 */
	bool handlePath(const std::string& path);

	/**
	 * Handles print request commands received via topicIn
	 * @param arg What to print. Accepted values are facts, rules
	 *            and agenda.
	 */
	bool handlePrint(const std::string& arg);

	/**
	 * Handles run request commands received via topicIn.
	 * On a successful parsing of the argument performs clips::run(arg)
	 * @param arg A string representation of an integer specifying
	 *            the maximum number of run steps to perform
	 */
	int handleRun(const std::string& arg);

	/**
	 * Handles toggle-watch request commands received via topicIn.
	 * On a successful parsing of the argument toggles the watching
	 * of functions, globals, facts or rules
	 * @param arg A string specifying which watch shall be toggled
	 */
	bool handleWatch(const std::string& arg);

	/**
	 * Parses command line arguments.
	 * Supported arguments are:
	 * -i   default input topic  (topicIn)
	 * -o   default output topic (topicOut)
	 * -s   default status_topic (topicStatus)
	 * -d   clp base path (where clips files are
	 * -e   File to load upon initialization
	 * -w   Indicates whether to watch facts upon initialization
	 * -r   Indicates whether to watch rules upon initialization
	 * @param  argc The main's argc
	 * @param  argv The main's argv
	 * @return      true if arguments were successfully parsed,
	 *              false otherwise
	 */
	bool parseArgs(int argc, char **argv);

	/**
	 * Prints the default arguments
	 * @param  pname The application name (receives argv[0])
	 */
	void printDefaultArgs(std::string const& pname);

	/**
	 * Prints the bridge's help message
	 * @param  pname The application name (receives argv[0])
	 */
	void printHelp(const std::string& pname);

	/**
	 * Handles incomming connections and starts an asynchronous accept again
	 * @param error      Error produced when accepting the connection
	 * @param socketPtr  A ponter to the socket used to connect with the remote client
	 *                   if the connection succeeded
	 */
	void acceptHandler(const boost::system::error_code& error, std::shared_ptr<boost::asio::ip::tcp::socket> socketPtr);

	/**
	 * Handles incomming data through a socket
	 * @param error Error produced when accepting the connection
	 * @param peer  The number of bytes transferred
	 */
	void asyncReadHandler(const boost::system::error_code& error, size_t bytes_transferred);

	/**
	 * Publishes a message to the default output topic (topicOut)
	 * @param  message The message to be published
	 * @return         true if the message was successfully published,
	 *                 false otherwise
	 */
	bool broadcast(const std::string& message);

	/**
	 * Sends a message to the specified client.
	 * @param  destPort   A string representation of the client's
	 *                    remote endpoint
	 * @param  message    The message to be published
	 * @return            true if the message was sent successfully,
	 *                    false otherwise
	 */
	bool sendTo(const std::string& cliEP, const std::string& message);

	/**
	 * Publishes the status of the bridge to topicStatus
	 * @return         true if the status was successfully published,
	 *                 false otherwise
	 */
	bool publishStatus();

	/**
	 * Prints all facts by calling clips::printFacts()
	 */
	void printFacts();

	/**
	 * Prints all facts by calling clips::printRules()
	 */
	void printRules();

	/**
	 * Prints all facts by calling clips::printAgenda()
	 */
	void printAgenda();


private:
	/**
	 * Friend function called by the homonymous registered CLIPS user-
	 * function when (sendto destport message) is invoked.
	 * @remark            The function shall return
	 *                    @c server.publish(topicName, message);
	 * @param  server     A reference to this server
	 * @param  cliEP      A string representation of the remote endoint of
	 *                    the client where the message will be delivered
	 * @param  message    The message to send
	 * @return            1 if the mesage was published, 0 otherwise
	 */
	friend int server_sendto_invoker(Server& server, const std::string& cliEP, const std::string& message);

	/**
	 * Friend function called by the homonymous registered CLIPS user-
	 * function when (broadcast message) is invoked.
	 * @remark            The function shall return
	 *                    @c server.subscribe(topicName, factName);
	 * @param  server     A reference to this server
	 * @param  message    The message to broadcast
	 * @return            1 if the subscription succeeded, 0 otherwise
	 */
	friend int server_broadcast_invoker(Server& server, const std::string& message);


protected:
	/**
	 * Stores the file that will be loaded into CLIPS during
	 * initialization.
	 */
	std::string clipsFile;

	/**
	 * When true, activates fact watching during initialization
	 */
	bool flgFacts;

	/**
	 * When true, activates defrule watching during initialization
	 */
	bool flgRules;

	/**
	 * Specifies the basepath where CLP files are located
	 */
	std::string clppath;

	/**
	 * Internal flag that keeps the bridge running.
	 * It is set to true by run() until changed to false by stop() or
	 * unless an external event modifies it.
	 */
	bool running;

	/**
	 * The syncrhonous queue used to pass messages to CLIPS.
	 * @remark CLIPS functions crash if called from a separate thread.
	 */
	sync_queue<std::shared_ptr<TcpMessage>> queue;

	/**
	 * Thread used to asynchronously run the bridge
	 */
	std::thread asyncThread;

	/**
	 * Stores the name of the topic this bridge listens to
	 */
	uint16_t port;

	/**
	 * Context required for async connections
	 */
	boost::asio::io_context io_context;

	/**
	 * Pointer to an acceptor objects that handles incomming connections
	 */
	std::shared_ptr<boost::asio::ip::tcp::acceptor> acceptorPtr;

	/**
	 * Stores the name of the fact where network messages are asserted.
	 */
	std::string defaultMsgInFact;

	/**
	 * Active connections to tcp clients
	 */
	// std::unordered_map<boost::asio::ip::tcp::endpoint, std::shared_ptr<boost::asio::ip::tcp::socket>> clients;
	std::unordered_map<std::string, std::shared_ptr<Session>> clients;


};

#endif // __CLIPS_BRIDGE_H__
