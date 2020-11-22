#include <kodi/addon-instance/PVR.h>
#include "rest.h"

class ATTRIBUTE_HIDDEN CPVRJellyfin : public kodi::addon::CAddonBase,
                                  public kodi::addon::CInstancePVRClient
{
public:
  CPVRJellyfin();
  ~CPVRJellyfin() override;

  PVR_ERROR GetBackendName(std::string& name) override;
  PVR_ERROR GetBackendVersion(std::string& version) override;

  PVR_ERROR GetCapabilities(kodi::addon::PVRCapabilities& capabilities) override;
 
  PVR_ERROR GetChannelsAmount(int& amount) override;
  PVR_ERROR GetChannels(bool bRadio, kodi::addon::PVRChannelsResultSet& results) override;

private:
  cJellyfinRest rest;
  std::string token;
  std::string userId;
  bool Login();
};