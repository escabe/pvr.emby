#include "client.h"
#include <kodi/General.h>
#include <json/json.h>
#include <utility>

CPVRJellyfin::CPVRJellyfin() {
}

CPVRJellyfin::~CPVRJellyfin() {

}

void CPVRJellyfin::ReadSettings() {
  username = kodi::GetSettingString("username","");
  password = kodi::GetSettingString("password","");
  baseUrl = kodi::GetSettingString("baseUrl","");
  verifyPeer = kodi::GetSettingBoolean("verifyPeer", true);
  rest.SetVerifyPeer(verifyPeer);
}

ADDON_STATUS CPVRJellyfin::Create()  {
  ReadSettings();
  if (username == "" || baseUrl == "") {
    kodi::QueueNotification(QUEUE_ERROR,"PVR Jellyfin","Add-on not configured. Please check the add-on settings.");
    return ADDON_STATUS_NEED_SETTINGS;
  }
  if (Login()) {
    authenticated = true;
    return ADDON_STATUS_OK;
  }
  return ADDON_STATUS_NEED_SETTINGS;
}

ADDON_STATUS CPVRJellyfin::SetSetting(const std::string &settingName, const kodi::CSettingValue &settingValue) {
  return ADDON_STATUS_NEED_RESTART;
}

bool CPVRJellyfin::Login() {
  int retval;
  
  Json::Value response;  
  // Build login data
  Json::Value data;  
  Json::FastWriter fastWriter;
  data["username"] = username;
  data["pw"] = password;
  std::string strdata = fastWriter.write(data);
  // Perform the request
  std::string strUrl = baseUrl + "/Users/AuthenticateByName";
  retval = this->rest.Post(strUrl, strdata, response);
  if (retval != E_FAILED)
  {
    // Store token and user Id
    token = response["AccessToken"].asString();
    userId = response["User"]["Id"].asString();
    return true;
  } else {
    kodi::QueueNotification(QUEUE_ERROR, "PVR Jellyfin","Failed to authenticate with server. Please check the plugin settings.");
  }
  return false;
}

PVR_ERROR CPVRJellyfin::GetCapabilities(kodi::addon::PVRCapabilities& capabilities) {
  if (!authenticated)
    return PVR_ERROR_SERVER_ERROR;
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
  amount = numChannels;
  return PVR_ERROR_NO_ERROR;
}
 
PVR_ERROR CPVRJellyfin::GetChannels(bool bRadio, kodi::addon::PVRChannelsResultSet& results)
{

  channelMap.clear();

  Json::Value data;
  if (rest.Get(baseUrl + "/LiveTv/Channels", "", data, token) == 0) {
    data = data["Items"];
    numChannels = data.size();
    for (unsigned int i = 0; i < data.size(); i++) {
      Json::Value entry = data[i];
      kodi::addon::PVRChannel kodiChannel;
      unsigned int channelNumber = std::stoul(entry["ChannelNumber"].asString());
      kodiChannel.SetUniqueId(channelNumber);
      kodiChannel.SetChannelNumber(channelNumber);
      kodiChannel.SetChannelName(entry["Name"].asString());
      channelMap.insert(std::make_pair(channelNumber,entry["Id"].asString()));
      results.Add(kodiChannel);
    }
    
  } else {
    return PVR_ERROR_SERVER_ERROR;
  }
  return PVR_ERROR_NO_ERROR;
}
 

ADDONCREATOR(CPVRJellyfin)