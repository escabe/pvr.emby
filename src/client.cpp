#include "client.h"

#include <json/json.h>

CPVRJellyfin::CPVRJellyfin() {
  // Login
  Login();
}

CPVRJellyfin::~CPVRJellyfin() {

}

ADDON_STATUS CPVRJellyfin::Create() {
  return ADDON_STATUS_NEED_SETTINGS;
}

ADDON_STATUS CPVRJellyfin::SetSetting(const std::string &settingName, const kodi::CSettingValue &settingValue) {

}

bool CPVRJellyfin::Login() {
  int retval;
  
  Json::Value response;  
  // Build login data
  Json::Value data;  
  Json::FastWriter fastWriter;
  data["username"] = "Martijn";
  data["pw"] = "";
  std::string strdata = fastWriter.write(data);
  // Perform the request
  std::string strUrl = "https://192.168.1.2:8920/Users/AuthenticateByName";
  retval = this->rest.Post(strUrl, strdata, response);
  if (retval != E_FAILED)
  {
    // Store token and user Id
    token = response["AccessToken"].asString();
    userId = response["User"]["Id"].asString();
    return true;
  }
  return false;
}

PVR_ERROR CPVRJellyfin::GetCapabilities(kodi::addon::PVRCapabilities& capabilities)
{
  capabilities.SetSupportsTV(true);
  return PVR_ERROR_NO_ERROR;
}
 
PVR_ERROR CPVRJellyfin::GetBackendName(std::string& name)
{
  name = "My special PVR client";
  return PVR_ERROR_NO_ERROR;
}
 
PVR_ERROR CPVRJellyfin::GetBackendVersion(std::string& version)
{
  version = "1.0.0";
  return PVR_ERROR_NO_ERROR;
}
 
PVR_ERROR CPVRJellyfin::GetChannelsAmount(int& amount)
{
  amount = 1;
  return PVR_ERROR_NO_ERROR;
}
 
PVR_ERROR CPVRJellyfin::GetChannels(bool bRadio, kodi::addon::PVRChannelsResultSet& results)
{
  kodi::addon::PVRChannel kodiChannel;
  kodiChannel.SetUniqueId(1);
  kodiChannel.SetChannelName("My Test Channel");
  results.Add(kodiChannel);
  return PVR_ERROR_NO_ERROR;
}
 

ADDONCREATOR(CPVRJellyfin)