#ifndef __REPLY_H__
#define __REPLY_H__
#pragma once

/** @cond */
#include <memory>
#include <string>

#include <boost/asio.hpp>
/** @endcond */

#include "request.h"

class Reply;
typedef std::shared_ptr<Reply> ReplyPtr;

class Reply{
private:
	Reply(uint32_t cmdId, bool success=0, const std::string& result="");
	Reply(Reply const& obj)        = delete;
	Reply& operator=(Reply const&) = delete;

public:
	uint32_t    getCommandId() const;
	bool        getSuccess() const;
	std::string getResult() const;

	bool matches(const Request& r);
	bool matches(const RequestPtr& r);

private:
	uint32_t cmdId;
	bool success;
	std::string result;

public:
	static bool matches(const Reply& rep, const Request& req);
	static bool matches(const ReplyPtr& rep, const RequestPtr& req);
	static ReplyPtr fromMessage(const std::string& message);

public:
	static const uint32_t CommandIdNone = -1;
};

#endif //__REPLY_H__
