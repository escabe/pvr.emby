#pragma once

#include <string>
#include <json/json.h>

#define E_SUCCESS 0
#define E_FAILED -1
#define E_EMPTYRESPONSE -2

class cJellyfinRest
{
public:
	cJellyfinRest(void) {};
	~cJellyfinRest(void) {};

	int Get(const std::string& command, const std::string& arguments, Json::Value& json_response,const std::string& token = std::string());
	int Post(const std::string& command, const std::string& arguments, Json::Value& json_response,const std::string& token = std::string());
	int Delete(const std::string& command, const std::string& arguments, const std::string& token = std::string());

private:
	std::string authorization;
	std::string getAuthorization();
	int httpRequest(const std::string& command, const std::string& arguments, const bool write, std::string& json_response, const std::string& token);
};
