#ifndef __REQUEST_H__
#define __REQUEST_H__
#pragma once

/** @cond */
#include <memory>
#include <string>
#include <vector>

#include <boost/asio.hpp>
/** @endcond */

class Request;
typedef std::shared_ptr<Request> RequestPtr;

class Request{
public:
	Request(const std::string& command, const std::string& args="");

private:
	Request();
	Request(Request const& obj)        = delete;
	Request& operator=(Request const&) = delete;

public:
	uint32_t    getCommandId() const;
	std::string getCommand() const;
	std::string getArgs() const;

	std::vector<char> getPayload() const;

private:
	uint32_t cmdId;
	std::string cmd;
	std::string args;

private:
	static uint32_t lastCommandId;

public:
	static RequestPtr fromMessage(const std::string& message);
	static RequestPtr make_shared(const std::string& command, const std::string& args="");

public:
	static const uint32_t CommandIdNone = -1;

};


#endif //__REQUEST_H__
