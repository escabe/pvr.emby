/*
*      Copyright (C) 2005-2010 Team XBMC
*      http://www.xbmc.org
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#include "rest.h"
#include "base64.h"
#include <memory>
#include <vector>
#include <stdlib.h>
#include <string.h>

using namespace ADDON;

int cRest::Get(const std::string& command, const std::string& arguments, Json::Value& json_response, const std::string& token)
{
	std::string response;
	int retval;
	retval = httpRequest(command, arguments, false, response,token);

	if (retval != E_FAILED)
	{
		if (response.length() != 0)
		{
			std::string jsonReaderError;
			Json::CharReaderBuilder jsonReaderBuilder;
			std::unique_ptr<Json::CharReader> const reader(jsonReaderBuilder.newCharReader());

			if (!reader->parse(response.c_str(), response.c_str() + response.size(), &json_response, &jsonReaderError))
			{
				XBMC->Log(LOG_DEBUG, "Failed to parse %s: \n%s\n",
					response.c_str(),
					jsonReaderError.c_str());
				return E_FAILED;
			}
		}
		else
		{
			XBMC->Log(LOG_DEBUG, "Empty response");
			return E_EMPTYRESPONSE;
		}
	}

	return retval;
}

int cRest::Post(const std::string& command, const std::string& arguments, Json::Value& json_response, const std::string& token)
{
	std::string response;
	int retval;
	retval = httpRequest(command, arguments, true, response, token);

	if (retval != E_FAILED)
	{
		if (response.length() != 0)
		{
			std::string jsonReaderError;
			Json::CharReaderBuilder jsonReaderBuilder;
			std::unique_ptr<Json::CharReader> const reader(jsonReaderBuilder.newCharReader());

			if (!reader->parse(response.c_str(), response.c_str() + response.size(), &json_response, &jsonReaderError))
			{
				XBMC->Log(LOG_DEBUG, "Failed to parse %s: \n%s\n",
					response.c_str(),
					jsonReaderError.c_str());
				return E_FAILED;
			}
		}
		else
		{
			XBMC->Log(LOG_DEBUG, "Empty response");
			return E_EMPTYRESPONSE;
		}
	}

	return retval;
}

int httpRequest(const std::string& command, const std::string& arguments, const bool write, std::string& json_response, const std::string& token)
{
	//P8PLATFORM::CLockObject critsec(communication_mutex);		
	std::string strUrl = command;
	
	if (write) {	// POST http request
		void* hFile = XBMC->CURLCreate(strUrl.c_str());
		std::string data = base64_encode(arguments.c_str(),arguments.length());
		XBMC->CURLAddOption(hFile,XFILE::CURL_OPTION_PROTOCOL,"postdata",data.c_str());
		XBMC->CURLAddOption(hFile,XFILE::CURL_OPTION_HEADER,"Authorization","MediaBrowser Client=\"KodiPVR\", Device=\"Ubuntu\", DeviceId=\"42\", Version=\"0.1.0\"");
		XBMC->CURLAddOption(hFile,XFILE::CURL_OPTION_HEADER,"Content-Type","application/json");
		if (!token.empty())
			XBMC->CURLAddOption(hFile,XFILE::CURL_OPTION_HEADER,"X-MediaBrowser-Token",token.c_str());
		XBMC->CURLOpen(hFile,0);
		if (hFile != NULL)
		{
			std::string result;
			result.clear();
			char buffer[1024];
			while (XBMC->ReadFileString(hFile, buffer, 1024))
				result.append(buffer);
			json_response = result;
			XBMC->CloseFile(hFile);
			return 0;
		}
	}
	else {	// GET http request	
		strUrl += arguments;

		void* fileHandle = XBMC->CURLCreate(strUrl.c_str());
		XBMC->CURLAddOption(fileHandle,XFILE::CURL_OPTION_HEADER,"Authorization","MediaBrowser Client=\"KodiPVR\", Device=\"Ubuntu\", DeviceId=\"42\", Version=\"0.1.0\"");
		if (!token.empty())
			XBMC->CURLAddOption(fileHandle,XFILE::CURL_OPTION_HEADER,"X-MediaBrowser-Token",token.c_str());
		XBMC->CURLOpen(fileHandle,0);

		if (fileHandle)
		{
			std::string result;
			char buffer[1024];
			while (XBMC->ReadFileString(fileHandle, buffer, 1024))
				result.append(buffer);

			XBMC->CloseFile(fileHandle);
			json_response = result;
			return 0;
		}
	}

	return -1;
}

