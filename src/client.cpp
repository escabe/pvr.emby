#include "client.h"
#include <kodi/General.h>
#include <kodi/Filesystem.h>
#include <json/json.h>
#include <utility>
#include <chrono>

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
    GetServerInfo();
    SetUUID();
    return ADDON_STATUS_OK;
  }
  return ADDON_STATUS_NEED_SETTINGS;
}

void CPVRJellyfin::SetUUID() {
  std::string path = UserPath();

  if (!kodi::vfs::DirectoryExists(path))
    kodi::vfs::CreateDirectory(path);
    
  std::string uuid;

  path.append("/uuid");
  if (kodi::vfs::FileExists(path)) {
    kodi::vfs::CFile file;
    file.OpenFile(path);
    file.ReadLine(uuid);
    file.Close();
  } else {
    GenerateUuid(uuid);
    kodi::vfs::CFile file;
    file.OpenFileForWrite(path, true);
    file.Write(uuid.c_str(),uuid.length());
    file.Close();
  }
  rest.SetUUID(uuid);
  
}

void CPVRJellyfin::GetServerInfo() {
  Json::Value response;  
  if (rest.Get(baseUrl + "/System/Info", "", response, token) == E_SUCCESS) {
    backendName = response["ServerName"].asString();
    backendVersion = response["Version"].asString();
  }
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
  name = backendName;
  return PVR_ERROR_NO_ERROR;
}
 
PVR_ERROR CPVRJellyfin::GetBackendVersion(std::string& version)
{
  version = backendVersion;
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
  if (rest.Get(baseUrl + "/LiveTv/Channels", "", data, token) == E_SUCCESS) {
    data = data["Items"];
    numChannels = data.size();
    for (unsigned int i = 0; i < data.size(); i++) {
      Json::Value entry = data[i];
      kodi::addon::PVRChannel kodiChannel;
      unsigned int channelNumber = std::stoul(entry["ChannelNumber"].asString());
      kodiChannel.SetUniqueId(channelNumber);
      kodiChannel.SetChannelNumber(channelNumber);
      kodiChannel.SetChannelName(entry["Name"].asString());
      kodiChannel.SetIconPath(baseUrl + "/Items/" + entry["Id"].asString() + "/Images/Primary");
      channelMap.insert(std::make_pair(channelNumber,entry["Id"].asString()));
      results.Add(kodiChannel);
    }
  } else {
    return PVR_ERROR_SERVER_ERROR;
  }
  return PVR_ERROR_NO_ERROR;
}
 
 void CPVRJellyfin::GenerateUuid(std::string& uuid)
{
  using namespace std::chrono;

  int64_t seed_value =
      duration_cast<milliseconds>(
          time_point_cast<milliseconds>(high_resolution_clock::now()).time_since_epoch())
          .count();
  seed_value = seed_value % 1000000000;
  srand((unsigned int)seed_value);

  //fill in uuid string from a template
  std::string template_str = "xxxx-xx-xx-xx-xxxxxx";
  for (size_t i = 0; i < template_str.size(); i++)
  {
    if (template_str[i] != '-')
    {
      double a1 = rand();
      double a3 = RAND_MAX;
      unsigned char ch = (unsigned char)(a1 * 255 / a3);
      char buf[16];
      sprintf(buf, "%02x", ch);
      uuid += buf;
    }
    else
    {
      uuid += '-';
    }
  }
}

ADDONCREATOR(CPVRJellyfin)