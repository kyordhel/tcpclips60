#ifndef __CLIPS_CLIENT_H__
#define __CLIPS_CLIENT_H__
#pragma once

/** @cond */
#include <map>
#include <mutex>
#include <string>
#include <iomanip>
#include <condition_variable>

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
/** @endcond */

#include "reply.h"
#include "clipsstatus.h"

class ClipsClient;
typedef std::shared_ptr<ClipsClient> ClipsClientPtr;

/**
 * Implements a tcp client that connects to clipsserver
 */
class ClipsClient: public std::enable_shared_from_this<ClipsClient>{
	struct Private{ explicit Private() = default; };
public:
	/**
	 * Initializes a new instance of ClipsClient.
	 */
	ClipsClient(const Private&);
	~ClipsClient();

private:
	// Disable copy constructor and assignment op.
	/**
	 * Copy constructor disabled
	 */
	ClipsClient(ClipsClient const& obj)        = delete;
	/**
	 * Copy assignment operator disabled
	 */
	ClipsClient& operator=(ClipsClient const&) = delete;

public:
	/**
	 * Connects to ClipsServer
	 * @param  address ClipsServer IPv4 address
	 * @param  port    ClipsServer port
	 * @return         true if a connection was established, false otherwise
	 */
	bool connect(const std::string& address, uint16_t port);

	/**
	 * Disconnects from ClipsServer
	 */
	void disconnect();

	/**
	 * Requests ClipsServer to load a file
	 * @param file Path to the clp file to load
	 */
	void loadFile(const std::string& file);

	/**
	 * Requests ClipsServer to execute the (clear) command
	 */
	void clear();

	/**
	 * Requests ClipsServer to execute the (reset) command
	 */
	void reset();

	/**
	 * Requests ClipsServer to run clips, executing the (run n) command
	 * @param n Maximum number fo rules to fire.
	 *          A negative value will fire all pending rules until the agenda becomes empty.
	 *          Default: -1
	 */
	void run(int32_t n = -1);

	/**
	 * Requests ClipsServer to execute the (assert fact) command
	 * @param fact The fact to assert
	 */
	void assertFact(const std::string& fact);

	/**
	 * Requests ClipsServer to execute the (retract fact) command
	 * @param fact The fact to retract
	 */
	void retractFact(const std::string& fact);

	/**
	 * Requests ClipsServer to execute a command.
	 * A command is any of
	 * 		assert   Asserts the fact given in args
	 * 		reset    Resets CLIPS
	 * 		clear    Clears CLIPS KB
	 * 		raw      Injects the string in CLIPS language contained in args
	 * 		path     Sets the working path of CLIPSServer
	 * 		print    Prints the elements specified in args (any of {facts, rules, agenda})
	 * 		watch    Toggles the watch set in args (any of {functions, globals, facts, rules})
	 * 		load     Loads the CLP or DAT file specidied in args
	 * 		run      Executes (run n) with the integer value given in args
	 * 		log      Sets the log level of CLIPSServer
	 *
	 * @param  cmd  The command to execute
	 * @param  args The command to execute
	 * @return      true if the command was successfully executed, false otherwise
	 */
	bool execute(const std::string& cmd, const std::string& args);

	/**
	 * Requests ClipsServer to perform a query on the KB
	 * @param  query  A string containing query to perform on CLIPS language
	 * @param  result The results yielded by CLIPS
	 * @return        true if the query was performed, false otherwise
	 */
	bool query(const std::string& query, std::string& result);

	/**
	 * Sends the given string to CLIPSServer
	 * @param s The string to send
	 */
	bool send(const std::string& s);

	/**
	 * REtrieves the latest reported status sent by CLIPSServer
	 * @return       The latest reported status sent by CLIPSServer
	 */
	ClipsStatusPtr getStatus();

	/**
	 * Requests ClipsServer to report the active watches
	 * @return       An integer containing CLIPS watches
	 */
	uint32_t getWatches();

	/**
	 * Requests ClipsServer to toggle a watch
	 * @param  watch Any of {functions, globals, facts, rules}
	 * @return       An integer containing CLIPS watches to toggle
	 */
	uint32_t toggleWatch(const std::string& watch);

public:
	ClipsClientPtr getPtr();

	void addMessageReceivedHandler(std::function<void(const ClipsClientPtr&, const std::string&)> handler);
	void addClipsStatusChangedHandler(std::function<void(const ClipsClientPtr&, const ClipsStatusPtr&)> handler);
	void addConnectedHandler(std::function<void(const ClipsClientPtr&)> handler);
	void addDisconnectedHandler(std::function<void(const ClipsClientPtr&)> handler);

#if __GNUC__ > 10
	void removeMessageReceivedHandler(std::function<void(const ClipsClientPtr&, const std::string&)> handler);
	void removeClipsStatusChangedHandler(std::function<void(const ClipsClientPtr&, const ClipsStatusPtr&)> handler);
	void removeConnectedHandler(std::function<void(const ClipsClientPtr&)> handler);
	void removeDisconnectedHandler(std::function<void(const ClipsClientPtr&)> handler);
#endif

protected:
	/**
	 * Sends the given string to CLIPSServer as a raw command to be executed by CLIPS
	 * @param s The string to send
	 */
	bool sendRaw(const std::string& s);


	/**
	 * Begins an asynchronous read operation
	 */
	void beginReceive();


	/**
	 * Handles asyncrhonous data reception
	 * @param error             Error code
	 * @param bytes_transferred Number of bytes transferred
	 */
	void asyncReadHandler(const boost::system::error_code& error, size_t bytes_transferred);

	/**
	 * Calls handles for received messages
	 * @param handler The received message string
	 */
	void onConnected();

	/**
	 * Calls handles for received messages
	 * @param handler The received message string
	 */
	void onDisconnected();

	/**
	 * Calls handles for received messages
	 * @param handler The received message string
	 */
	void onMessageReceived(const std::string& s);

	/**
	 * Calls handles for clips status change
	 * @param handler The new status
	 */
	void onClipsStatusChanged();

private:
	/**
	 * Sends the given command to ClipsServer
	 * @param command The command to send
	 * @param args    The arguments for the command
	 */
	bool sendCommand(const std::string& command, const std::string& args, uint32_t& cmdId);

	/**
	 * Awaits until a response arrives from the server
	 * @param cmdId   The ID of the command that awaits for response
	 * @param success When this method returns contains a boolean indicating
	 *                whether the execution on the remote server was successful
	 * @param result  When this method returns contains the results produced
	 *                by the rpc call on the remote server
	 */
	bool awaitResponse(int cmdId, bool& success, std::string& result);

	/**
	 * Performs a RPC call on CLIPSServer to execute a command and synchronously
	 * awaits for the response to arrive
	 * @param cmd     The command to send and execute
	 * @param args    The arguments for the command
	 * @param result  When this method returns contains the results produced
	 *                by the rpc call on the remote server
	 * @return        true if the RPC was successfully completed, false otherwise.
	 */
	bool rpc(const std::string& cmd, const std::string& args, std::string& result);
	bool rpc(const std::string& cmd);
	bool rpc(const std::string& cmd, const std::string& args);

	/**
	 * Aborts all RPC request releasing all waiting locks. To be used during disconnection.
	 */
	void abortAllRPC();

	/**
	 * Analyzes a received response and triggers the mechanisms to complete RPCs for issued commands
	 * @param s The received message
	 */
	void handleResponseMesage(const std::string& s);

	/**
	 * Checks whether the response for a given command has arrived.
	 * For use of a condition variable
	 * @param  cmdId   The id of the command
	 * @param  aborted A value indicating whether the call was aborted (e.g. disconnection)
	 * @return         true if the response has arrived, false otherwise
	 */
	bool hasReponseArrived(uint32_t cmdId, bool& aborted);

	/**
	 * Updates the status based on the info sent by CLIPSServer
	 * @param ReplyPtr A pointer to the reply object containing the status
	 */
	void updateStatus(ReplyPtr);

private:
	/**
	 * Service required for async communications
	 */
	boost::asio::io_service io_service;

	/**
	 * Background thread to run the service
	 */
	std::shared_ptr<boost::thread> serviceThreadPtr;

	/**
	 * Pointer to the socket object used to connect to the clips server
	 */
	std::shared_ptr<boost::asio::ip::tcp::socket> socketPtr;

	/**
	 * Buffer to receive messages asynchronously
	 */
	boost::asio::streambuf buffer;

	/**
	 * Stream used to read the buffer
	 */
	std::istream is;

	/**
	 * Protection lock for the pendingCommands map
	 */
	std::mutex pcmutex;

	/**
	 * Condition variable to notify RPC waiters their response has arrived
	 */
	std::condition_variable pccv;

	/**
	 * Stores the ids of commands sent that are awaiting for a response;
	 * as well as the response when it arrives
	 */
	std::map<uint32_t, ReplyPtr> pendingCommands;

	/**
	 * Stores handler functions for message reception
	 */
	std::vector<std::function<void(const ClipsClientPtr&, const std::string&)>> messageReceivedHandlers;

	/**
	 * Stores handler functions for status changed event
	 */
	std::vector<std::function<void(const ClipsClientPtr&, const ClipsStatusPtr&)>> clipsStatusChangedHandlers;

	/**
	 * Stores handler functions for connect events
	 */
	std::vector<std::function<void(const ClipsClientPtr&)>> connectedHandlers;

	/**
	 * Stores handler functions for disconnection events
	 */
	std::vector<std::function<void(const ClipsClientPtr&)>> disconnectedHandlers;

	/**
	 * Stores CLIPS status and active watches
	 */
	ClipsStatusPtr clipsStatus;


// Facotry functions replace constructor
public:
	/**
	 * Initializes a new instance of ClipsClient and returns a shared pointer to it.
	 * @return A shared pointer to a newly created instance of ClipsClient
	 */
	static ClipsClientPtr create();
};

#endif // __CLIPS_CLIENT_H__

