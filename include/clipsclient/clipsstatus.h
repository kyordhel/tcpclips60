#ifndef __CLIPS_STATUS_H__
#define __CLIPS_STATUS_H__
#pragma once

#include<memory>
#include<string>

class ClipsStatus;
typedef std::shared_ptr<ClipsStatus> ClipsStatusPtr;

class ClipsStatus{
private:
	ClipsStatus();

public:
	uint32_t getWatches();
	std::string getClpPath();

private:
	uint32_t watches;
	std::string clpPath;

public:
	static ClipsStatusPtr fromString(const std::string& s);
};

#endif // __CLIPS_STATUS_H__
