/* ** *****************************************************************
* session.h
*
* Author: Mauricio Matamoros
*
* ** *****************************************************************/
/** @file session.h
 * Definition of a session: represents an active connection between
 * a client and the TCP server
 */

#ifndef __SESSION_H__
#define __SESSION_H__
#pragma once

/** @cond */
#include <string>
#include <iomanip>
#include <boost/asio.hpp>
/** @endcond */

#include "tcp_message.h"



class Server;

class Session{
public:
	/**
	 * Initializes a new instance of Session
	 * @param socketPtr    The underlaying connection socket to the remote client.
	 * @param server       The server that manages the session and handles incomming messages.
	 */
	Session(
		std::shared_ptr<boost::asio::ip::tcp::socket> socketPtr,
		Server& serverPtr
	);
	~Session();

	// Disable copy constructor and assignment op.
private:
	/**
	 * Copy constructor disabled
	 */
	Session(Session const& obj)        = delete;
	/**
	 * Copy assignment operator disabled
	 */
	Session& operator=(Session const&) = delete;

public:
	/**
	 * Gets a string representation of the remote endpoint
	 * @return A string representation of the remote endpoint
	 */
	std::string getEndPointStr() const;

	/**
	 * Gets the underlaying connection socket to the remote client
	 * @return The underlaying connection socket to the remote client
	 */
	std::shared_ptr<boost::asio::ip::tcp::socket> getSocketPtr() const;


public:
	/**
	 * Sends the provided string to the remote client
	 * @param s The string to send
	 */
	void send(const std::string& s);


private:
	/**
	 * Start an asynchronous operation poll to receive data from the remote client.
	 * After each reception a new asynchronous reception operation is started automatically.
	 * @param queue    A queue to store incomming messages.
	 */
	void beginAsyncReceivePoll();


	/**
	 * Handles incomming data through a socket
	 * @param error Error produced when accepting the connection
	 * @param peer  The number of bytes transferred
	 */
	void asyncReadHandler(const boost::system::error_code& error, size_t bytes_transferred);

	/**
	 * Verifies whether all data has been received
	 * @param error Error produced when accepting the connection
	 * @param peer  The number of bytes transferred
	 */
	size_t checkTransferComplete(const boost::system::error_code& error, size_t bytes_transferred);

	/**
	 * Reads the header of the message from the buffer without advancing the read cursor.
	 * @param is A istream to read the buffer
	 */
	uint16_t peekMessageHeader(std::istream& is);

	/**
	 * Reads the a message from the buffer.
	 * @param is A istream to read the buffer
	 */
	std::string fetchStringFromBuffer(std::istream& is);


private:
	/**
	 * Stores a string representation of the
	 */
	std::string endpoint;

	/**
	 * Dynamic buffer required to receive messages asynchronously
	 */
	boost::asio::streambuf buffer;

	/**
	 * The underlaying connection socket to the remote client
	 */
	std::shared_ptr<boost::asio::ip::tcp::socket> socketPtr;

	/**
	 * The sessions lord and master
	 */
	Server& server;


public:
	/**
	 * Returns a shared pointer to a new instance of Session
	 * @param socketPtr The underlaying connection socket to the remote client
	 * @param server    The server that manages the session and handles incomming messages.
	 */
	static std::shared_ptr<Session> makeShared(
			std::shared_ptr<boost::asio::ip::tcp::socket> socketPtr,
			Server& server
	);
};

#endif
