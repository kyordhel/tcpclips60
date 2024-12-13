/* ** *****************************************************************
* tcp_message.h
*
* Author: Mauricio Matamoros
*
* ** *****************************************************************/
/** @file session.h
 * Definition of a session: represents an active connection between
 * a client and the TCP server
 */

#ifndef __TCP_MESSAGE_H__
#define __TCP_MESSAGE_H__
#pragma once

/** @cond */
#include <memory>
#include <string>
/** @endcond */

class TcpMessage{
	/**
	 * Initializes a new instance of TcpMessage
	 */
	TcpMessage(const std::string& source, const std::string& message);

	// Disable copy constructor and assignment op.
private:
	/**
	 * Copy constructor disabled
	 */
	TcpMessage(TcpMessage const& obj)        = delete;
	/**
	 * Copy assignment operator disabled
	 */
	TcpMessage& operator=(TcpMessage const&) = delete;

public:
	/**
	 * Retrieves the mesage source. Typically a string
	 * representation of the remote endpoint of the network
	 * client that sends the message
	 * @return The message source
	 */
	std::string& getSource();

	/**
	 * Retrieves the message contained in the packet
	 * @return The message contained in the packet
	 */
	std::string& getMessage();

private:
	/**
	 * The message source. Typically a string representation of the
	 * remote endpoint of the network client that sends the message
	 */
	std::string source;
	/**
	 * The message itself
	 */
	std::string message;


public:
	/**
	 * Returns a shared pointer to a new instance of TcpMessage
	 * @param source    The message source
	 * @param message   The message itself
	 */
	static std::shared_ptr<TcpMessage> makeShared(const std::string& source, const std::string& message);

};

#endif // __TCP_MESSAGE_H__