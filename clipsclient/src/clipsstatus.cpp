#include "clipsstatus.h"

#include <regex>


ClipsStatus::ClipsStatus(): watches(-1), clpPath(){}

uint32_t ClipsStatus::getWatches(){
	return watches;
}

std::string ClipsStatus::getClpPath(){
	return clpPath;
}

ClipsStatusPtr ClipsStatus::fromString(const std::string& s){
	static std::regex rxStatus("watching:(\\d{1,5})(\\|path:[^\\|]+)?", std::regex_constants::ECMAScript | std::regex_constants::optimize);
	std::smatch match;

	if(!std::regex_match(s, match, rxStatus) || match.empty()) { printf("Regex does not match\n");return NULL;}
	ClipsStatusPtr csptr(new ClipsStatus());
	csptr->watches = std::stoi(match[1].str());
	for(size_t i = 2; i < match.size(); ++i){
		if ( (match[i].str().length() > 6) && (match[i].str().rfind("|path:", 0) == 0) )
			csptr->clpPath = match[i].str().substr(6);
	}
	return csptr;
}
