#include "rest.h"
#include "base64.h"

#include <kodi/Filesystem.h>

int CJellyfinRest::Get(const std::string& command, const std::string& arguments, Json::Value& json_response, const std::string& token)
{
	std::string response;
	int retval;
	retval = this->httpRequest(command, arguments, false, response,token);

	if (retval != E_FAILED)
	{
		if (response.length() != 0)
		{
			std::string jsonReaderError;
			Json::CharReaderBuilder jsonReaderBuilder;
			std::unique_ptr<Json::CharReader> const reader(jsonReaderBuilder.newCharReader());

			if (!reader->parse(response.c_str(), response.c_str() + response.size(), &json_response, &jsonReaderError))
			{
				kodi::Log(AddonLog::ADDON_LOG_ERROR, "Failed to parse %s: \n%s\n",
					response.c_str(),
					jsonReaderError.c_str());
				return E_FAILED;
			}
		}
		else
		{
			kodi::Log(AddonLog::ADDON_LOG_DEBUG, "Empty response");
			return E_EMPTYRESPONSE;
		}
	}

	return retval;
}

int CJellyfinRest::Post(const std::string& command, const std::string& arguments, Json::Value& json_response, const std::string& token)
{
	std::string response;
	int retval;
	retval = this->httpRequest(command, arguments, true, response, token);

	if (retval != E_FAILED)
	{
		if (response.length() != 0)
		{
			std::string jsonReaderError;
			Json::CharReaderBuilder jsonReaderBuilder;
			std::unique_ptr<Json::CharReader> const reader(jsonReaderBuilder.newCharReader());

			if (!reader->parse(response.c_str(), response.c_str() + response.size(), &json_response, &jsonReaderError))
			{
				kodi::Log(AddonLog::ADDON_LOG_DEBUG, "Failed to parse %s: \n%s\n",
					response.c_str(),
					jsonReaderError.c_str());
				return E_FAILED;
			}
		}
		else
		{
			kodi::Log(ADDON_LOG_DEBUG, "Empty response");
			return E_EMPTYRESPONSE;
		}
	}

	return retval;
}


int CJellyfinRest::Delete(const std::string& strUrl, const std::string& arguments, const std::string& token)
{
	std::string response;
	int retval;
	kodi::vfs::CFile hFile;
	hFile.CURLCreate(strUrl);
	hFile.CURLAddOption(CURLOptiontype::ADDON_CURL_OPTION_PROTOCOL,"customrequest","DELETE");
	hFile.CURLAddOption(CURLOptiontype::ADDON_CURL_OPTION_HEADER,"Authorization",this->getAuthorization().c_str());
	if (!verifyPeer)
		hFile.CURLAddOption(CURLOptiontype::ADDON_CURL_OPTION_PROTOCOL,"verifypeer","false");
	if (!token.empty())
		hFile.CURLAddOption(CURLOptiontype::ADDON_CURL_OPTION_HEADER,"X-MediaBrowser-Token",token.c_str());
	if (hFile.CURLOpen())
	{
		hFile.Close();
		return 0;
	}
	return retval;
}

int CJellyfinRest::httpRequest(const std::string& command, const std::string& arguments, const bool write, std::string& json_response, const std::string& token)
{
	//P8PLATFORM::CLockObject critsec(communication_mutex);		
	std::string strUrl = command;
	
	if (write) {	// POST http request
		kodi::vfs::CFile hFile;
		hFile.CURLCreate(strUrl);
		std::string data = base64_encode(arguments.c_str(),arguments.length());
		hFile.CURLAddOption(CURLOptiontype::ADDON_CURL_OPTION_PROTOCOL,"postdata",data);
		if (!verifyPeer)
			hFile.CURLAddOption(CURLOptiontype::ADDON_CURL_OPTION_PROTOCOL,"verifypeer","false");
		hFile.CURLAddOption(CURLOptiontype::ADDON_CURL_OPTION_HEADER,"Authorization",this->getAuthorization());
		hFile.CURLAddOption(CURLOptiontype::ADDON_CURL_OPTION_HEADER,"Content-Type","application/json");
		if (!token.empty())
			hFile.CURLAddOption(CURLOptiontype::ADDON_CURL_OPTION_HEADER,"X-MediaBrowser-Token",token);

		if (hFile.CURLOpen())
		{
			std::string result;
			result.clear();
			char buffer[1024];
			while (hFile.Read(buffer, 1024))
				result.append(buffer);
			json_response = result;
			hFile.Close();
			return 0;
		}
	}
	else {	// GET http request	
		strUrl += arguments;
		kodi::vfs::CFile hFile;
		hFile.CURLCreate(strUrl);
		hFile.CURLAddOption(CURLOptiontype::ADDON_CURL_OPTION_HEADER,"Authorization",this->getAuthorization());
		if (!verifyPeer)
			hFile.CURLAddOption(CURLOptiontype::ADDON_CURL_OPTION_PROTOCOL,"verifypeer","false");
		if (!token.empty())
			hFile.CURLAddOption(CURLOptiontype::ADDON_CURL_OPTION_HEADER,"X-MediaBrowser-Token",token);

		if (hFile.CURLOpen())
		{
			std::string result;
			char buffer[1024];
			while (hFile.Read(buffer, 1024))
				result.append(buffer);

			hFile.Close();
			json_response = result;
			return 0;
		}
	}

	return -1;
}

std::string CJellyfinRest::getAuthorization() {
	
	if (this->authorization.empty()) {
		// Form full string
		this->authorization = kodi::tools::StringUtils::Format("MediaBrowser Client=\"Kodi PVR\", Device=\"Kodi\", DeviceId=\"%s\", Version=\"0.1.0\"","7f8217cd-9ebd-4c3d-824a-f58df585a323");
	}
	return this->authorization;
}

void CJellyfinRest::SetVerifyPeer(bool val) {
	verifyPeer = val;
}