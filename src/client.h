#include <kodi/addon-instance/PVR.h>
#include "rest.h"

class ATTRIBUTE_HIDDEN CPVRJellyfin : public kodi::addon::CAddonBase,
                                  public kodi::addon::CInstancePVRClient
{
public:
  CPVRJellyfin();
  ~CPVRJellyfin() override;
  ADDON_STATUS SetSetting(const std::string &settingName, const kodi::CSettingValue &settingValue) override;
  ADDON_STATUS Create() override;

  PVR_ERROR GetBackendName(std::string& name) override;
  PVR_ERROR GetBackendVersion(std::string& version) override;
  
  PVR_ERROR GetCapabilities(kodi::addon::PVRCapabilities& capabilities) override;
 
  PVR_ERROR GetChannelsAmount(int& amount) override;
  PVR_ERROR GetChannels(bool bRadio, kodi::addon::PVRChannelsResultSet& results) override;

private:
  CJellyfinRest rest;
  std::string token;
  std::string userId;
  std::string username;
  std::string password;
  std::string baseUrl;
  std::string backendName = "";
  std::string backendVersion = "";
  std::map<unsigned int,std::string> channelMap;
  bool verifyPeer = true;
  bool Login();
  bool authenticated = false;
  int numChannels = -1;
  void ReadSettings();
  void GetServerInfo();
  void SetUUID();
  void GenerateUuid(std::string& uuid);
};