#pragma once

#include <string>
#include <json/json.h>

#define E_SUCCESS 0
#define E_FAILED -1
#define E_EMPTYRESPONSE -2

class CJellyfinRest
{
public:
	CJellyfinRest(void) {};
	~CJellyfinRest(void) {};

	int Get(const std::string& command, const std::string& arguments, Json::Value& json_response,const std::string& token = std::string());
	int Post(const std::string& command, const std::string& arguments, Json::Value& json_response,const std::string& token = std::string());
	int Delete(const std::string& command, const std::string& arguments, const std::string& token = std::string());

	void SetVerifyPeer(bool val);
	void SetUUID(std::string newGuid);

private:
	std::string authorization;
	std::string uuid = "7f8217cd-9ebd-4c3d-824a-f58df585a323";
	std::string getAuthorization();
	bool verifyPeer = true;
	int httpRequest(const std::string& command, const std::string& arguments, const bool write, std::string& json_response, const std::string& token);
};
