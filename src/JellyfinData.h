#pragma once

//#include <vector>
#include "client.h"
#include "rest.h"
#include "p8-platform/threads/threads.h"
//#include "tinyxml/tinyxml.h"
#include <json/json.h>

#define Jellyfin_REST_INTERFACE false

#define CHANNELDATAVERSION  2

class CCurlFile
{
public:
  CCurlFile(void) {};
  ~CCurlFile(void) {};

  bool Get(const std::string &strURL, std::string &strResult);
};

typedef enum Jellyfin_UPDATE_STATE
{
  Jellyfin_UPDATE_STATE_NONE,
  Jellyfin_UPDATE_STATE_FOUND,
  Jellyfin_UPDATE_STATE_UPDATED,
  Jellyfin_UPDATE_STATE_NEW
} Jellyfin_UPDATE_STATE;


struct JellyfinChannel
{
  bool        bRadio;
  unsigned int iUniqueId;
  int         iChannelNumber;
  int		  iSubChannelNumber;
  int         iEncryptionSystem;  
  std::string strChannelName;
  std::string strLogoPath;
  std::string strStreamURL; 
  std::string strJellyfinId; 

  bool operator < (const JellyfinChannel& channel) const
  {
	  return (strChannelName.compare(channel.strChannelName) < 0);
  }
};

struct JellyfinChannelGroup
{
  bool              bRadio;
  int               iGroupId;
  std::string       strGroupName;
  std::vector<int>  members;
};

struct JellyfinEpgEntry
{
  unsigned int iBroadcastId;
  std::string strJellyfinBroadcastId;
  unsigned int iChannelId;
  std::string strJellyfinChannelId;
};

struct JellyfinEpgChannel
{
  std::string               strId;
  std::string               strName;
  std::vector<JellyfinEpgEntry> epg;
};

struct JellyfinTimer
{
  unsigned int    iId;
  std::string     strTitle;
  unsigned int    iChannelId;
  unsigned int    iProgramId;
  std::string     strJellyfinId;
  time_t          startTime;
  time_t          endTime;
  int             iStartOffset;
  int             iEndOffset;
  std::string     strProfile;
  std::string     strResult;
  PVR_TIMER_STATE state;  
};

struct JellyfinRecording
{
  std::string strRecordingId;
  time_t      startTime;
  int         iDuration;
  int         iLastPlayedPosition;
  std::string strTitle;
  std::string strStreamURL;
  std::string strPlot;
  std::string strPlotOutline;
  std::string strChannelName;
  std::string strDirectory;
  std::string strIconPath;
};

class Jellyfin : public P8PLATFORM::CThread
{
public:
  /* Class interface */
  Jellyfin(void);
  ~Jellyfin();

  /* Server */
  bool Open();  
  bool IsConnected();
  
  /* Common */
  const char* GetBackendName(void);
  const char* GetBackendVersion(void);

  /* Channels */
  unsigned int GetChannelsAmount(void);
  bool GetChannel(const PVR_CHANNEL &channel, JellyfinChannel &myChannel);
  PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);
  PVR_ERROR GetChannelStreamProperties(const PVR_CHANNEL* channel, PVR_NAMED_VALUE* properties, unsigned int* iPropertiesCount);

  /* Recordings */
  PVR_ERROR GetRecordings(ADDON_HANDLE handle);
  bool GetRecordingFromLocation(std::string strRecordingFolder);
  unsigned int GetRecordingsAmount(void);
  PVR_ERROR GetRecordingStreamProperties(const PVR_RECORDING* recording, PVR_NAMED_VALUE* properties, unsigned int* iPropertiesCount);

  /* Timer */
  unsigned int GetTimersAmount(void);
  PVR_ERROR GetTimers(ADDON_HANDLE handle);
  PVR_ERROR AddTimer(const PVR_TIMER &timer);    
  PVR_ERROR DeleteTimer(const PVR_TIMER &timer);

  /* EPG */
  PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd);  
  bool GetEPG(std::string id, time_t iStart, time_t iEnd, Json::Value& data);
  
  /* Preview */
  const char* GetLiveStreamURL(const PVR_CHANNEL &channelinfo);
  void CloseLiveStream();
  long long SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */);
  long long PositionLiveStream(void);
  long long LengthLiveStream(void);
  int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize);
  
  /* Storage */
  PVR_ERROR GetStorageInfo(long long *total, long long *used);
  
private:
  cRest rest;
  // helper functions    
  bool LoadChannels();  
  bool GetFreeConfig();
  unsigned int GetEventId(long long EntryId);
  /*
  * \brief Get a channel list from Jellyfin Device via REST interface
  * \param id The channel list id
  */
  int RESTGetChannelList(Json::Value& response);
  int RESTGetRecordings(Json::Value& response);
  int RESTGetTimer(Json::Value& response);  
  int RESTGetEpg(std::string id, time_t iStart, time_t iEnd, Json::Value& response);
  int RESTDeleteTimer(std::string id);
  int RESTAddTimer(std::string id, Json::Value& response);  

  // helper functions    
  std::string URLEncodeInline(const std::string& sSrc);
  void TransferChannels(ADDON_HANDLE handle);
  void TransferRecordings(ADDON_HANDLE handle);
  void TransferTimer(ADDON_HANDLE handle); 
  std::string GetStreamUrl(std::string id, std::string params);
  std::string GetChannelLogo(std::string params);
  bool Login(void);
  time_t ISO8601ToTime(const char* date);
  void GetServerInfo(void);
  void *Process(void);
    
  // members
  P8PLATFORM::CMutex                  m_mutex;
  P8PLATFORM::CCondition<bool>        m_started;

  bool                              m_bIsConnected;  
  std::string                       m_strHostname;
  std::string                       m_strBaseHTTPUrl;
  std::string                       m_strBaseHTTPSUrl;
  std::string                       m_strBackendName;
  std::string                       m_strBackendVersion;
  std::string                       m_strUsername;
  std::string                       m_strPassword;
  std::string                       m_strToken;
  std::string                       m_strUserId;

  std::string                       m_strLiveTVParameters;
  std::string                       m_strRecordingParameters;

  int                               m_iPortHTTP;    
  int                               m_iPortHTTPS;    
  int                               m_iNumChannels;
  int                               m_iNumRecordings;  
  bool                              m_bUpdating;  
  
  std::vector<JellyfinEpgEntry>       m_epg;    
  std::vector<JellyfinChannel>          m_channels;  
  std::vector<JellyfinRecording>        m_recordings; 
  std::vector<JellyfinTimer>            m_timer;
  std::vector<std::string>			m_partitions;

};


