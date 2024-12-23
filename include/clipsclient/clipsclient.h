#ifndef __CLIPS_CLIENT_H__
#define __CLIPS_CLIENT_H__
#pragma once

/** @cond */
#include <string>
#include <iomanip>

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
/** @endcond */

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
	 * Sends the given string to CLIPSServer
	 * @param s The string to send
	 */
	bool send(const std::string& s);

public:
	ClipsClientPtr getPtr();

	void addMessageReceivedHandler(std::function<void(const ClipsClientPtr&, const std::string&)> handler);
	void addConnectedHandler(std::function<void(const ClipsClientPtr&)> handler);
	void addDisconnectedHandler(std::function<void(const ClipsClientPtr&)> handler);

	void removeMessageReceivedHandler(std::function<void(const ClipsClientPtr&, const std::string&)> handler);
	void removeConnectedHandler(std::function<void(const ClipsClientPtr&)> handler);
	void removeDisconnectedHandler(std::function<void(const ClipsClientPtr&)> handler);

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
	 * Stores handler functions for message reception
	 */
	std::vector<std::function<void(const ClipsClientPtr&, const std::string&)>> messageReceivedHandlers;
	std::vector<std::function<void(const ClipsClientPtr&)>> connectedHandlers;
	std::vector<std::function<void(const ClipsClientPtr&)>> disconnectedHandlers;




// Facotry functions replace constructor
public:
	/**
	 * Initializes a new instance of ClipsClient and returns a shared pointer to it.
	 * @return A shared pointer to a newly created instance of ClipsClient
	 */
	static ClipsClientPtr create();
};

#endif // __CLIPS_CLIENT_H__

