#include "EmbyData.h"

#include "client.h"

#include <p8-platform/util/StringUtils.h>

#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <map>

#include "md5.h"


#define URI_REST_AUTHENTICATEBYNAME "/Users/AuthenticateByName"
#define URI_REST_CONFIG         "/TVC/free/data/config"
#define URI_REST_CHANNELS       "/LiveTv/Channels"
#define URI_REST_RECORDINGS     "/LiveTV/Recordings"
#define URI_REST_TIMER          "/LiveTV/Timers"
#define URI_REST_EPG            "/LiveTv/Programs"
#define URI_REST_STORAGE        "/TVC/user/data/storage"
#define URI_REST_FOLDER	        "/TVC/user/data/folder"

#define DEFAULT_TV_PIN          "0000"

#define URI_INDEX_HTML			"/TVC/common/Login.html"

#define DEFAULT_PREVIEW_MODE    "m2ts"
#define DEFAULT_PROFILE         "m2ts.Native.NR"
#define DEFAULT_REC_PROFILE     "m2ts.4000k.HR"

using namespace std;
using namespace ADDON;
using namespace P8PLATFORM;



/************************************************************/
/** Class interface */

Emby::Emby() :m_strBaseUrl(""), m_strToken("")
{   
  m_iPortWeb = g_iPortWeb;
  m_bIsConnected = false;      
  m_bUpdating = false;  
  m_iNumChannels = 0;
  m_iNumRecordings = 0;  
  m_strUsername = g_strUsername;
  m_strPassword = g_strPassword;
  m_strLiveTVParameters = g_strLiveTVParameters;
  m_strRecordingParameters = g_strRecordingParameters;
}

void  *Emby::Process()
{
  XBMC->Log(LOG_DEBUG, "%s - starting", __FUNCTION__);

  CLockObject lock(m_mutex);
  m_started.Broadcast();

  return NULL;
}

Emby::~Emby()
{
  CLockObject lock(m_mutex);
  XBMC->Log(LOG_DEBUG, "%s Stopping update thread...", __FUNCTION__);
  StopThread();

  XBMC->Log(LOG_DEBUG, "%s Removing internal channels list...", __FUNCTION__);
  m_channels.clear();
  m_epg.clear();
  m_recordings.clear();
  m_timer.clear();
  m_partitions.clear();
  m_bIsConnected = false;
  
}

bool Emby::Open()
{
  CLockObject lock(m_mutex);

  XBMC->Log(LOG_NOTICE, "%s - Emby Systems Addon Configuration options", __FUNCTION__);
  XBMC->Log(LOG_NOTICE, "%s - Hostname: '%s'", __FUNCTION__, g_strHostname.c_str());
  XBMC->Log(LOG_NOTICE, "%s - WebPort: '%d'", __FUNCTION__, m_iPortWeb);

  // Set base url
  std::string strURL = "";
  strURL= StringUtils::Format("http://%s%s:%u", strURL.c_str(), g_strHostname.c_str(), m_iPortWeb);
  m_strBaseUrl = strURL;

// Perform login
  m_bIsConnected = Login();
  if (!m_bIsConnected) {
    XBMC->Log(LOG_ERROR, "%s It seem's that Emby cannot be reached. Make sure that you set the correct configuration options in the addon settings!", __FUNCTION__);
    return false;
  }

  if (m_channels.size() == 0)
  {
    // Load the TV channels
	LoadChannels();    
  }  
    
  XBMC->Log(LOG_INFO, "%s Starting separate client update thread...", __FUNCTION__);
  CreateThread();

  return IsRunning();
}

bool Emby::Login(void) {
  int retval;
  cRest rest;
  Json::Value response;  
  // Build login data
  Json::Value data;  
  Json::FastWriter fastWriter;
  data["username"] = m_strUsername;
  data["pw"] = m_strPassword;
  std::string strdata = fastWriter.write(data);
  // Perform the request
  std::string strUrl = m_strBaseUrl + URI_REST_AUTHENTICATEBYNAME;
  retval = rest.Post(strUrl, strdata, response);
  if (retval != E_FAILED)
  {
    // Store token and user Id
    m_strToken = response["AccessToken"].asString();
    m_strUserId = response["User"]["Id"].asString();
    return true;
  }
  return false;
}

void Emby::CloseLiveStream(void)
{
  XBMC->Log(LOG_DEBUG, "CloseLiveStream");  
}

/************************************************************/
/** Channels  */
PVR_ERROR Emby::GetChannels(ADDON_HANDLE handle, bool bRadio) 
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);
  m_iNumChannels = 0;
  m_channels.clear();

  Json::Value data;
  int retval;
  retval = RESTGetChannelList(data);
  if (retval < 0)
  {
    XBMC->Log(LOG_ERROR, "No channels available.");
    return PVR_ERROR_SERVER_ERROR;
  }
  
  data = data["Items"];
  for (unsigned int index = 0; index < data.size(); ++index)
  {
    EmbyChannel channel;
    Json::Value entry;

    entry = data[index];
    
    channel.strEmbyId = entry["Id"].asString();
    channel.iUniqueId = *((unsigned int*)entry["Id"].asCString());
    channel.strChannelName = entry["Name"].asString();  
    channel.iChannelNumber = stoi(entry["ChannelNumber"].asString());
    channel.iSubChannelNumber = 0;
    channel.iEncryptionSystem = 0;	

    channel.strStreamURL = GetStreamUrl(entry["Id"].asString(),m_strLiveTVParameters);    
    channel.strLogoPath = GetChannelLogo(entry["Id"].asString());

    m_iNumChannels++;
    m_channels.push_back(channel);
    
    XBMC->Log(LOG_DEBUG, "%s loaded Channel entry '%s'", __FUNCTION__, channel.strChannelName.c_str());
  }
  
  if (m_channels.size() > 0) {
	  std::sort(m_channels.begin(), m_channels.end());
  }
	  
  XBMC->QueueNotification(QUEUE_INFO, "%d channels loaded.", m_channels.size());
  
  TransferChannels(handle);

  return PVR_ERROR_NO_ERROR;
}


void Emby::TransferChannels(ADDON_HANDLE handle)
{
  for (unsigned int i = 0; i < m_channels.size(); i++)
  {
    std::string strTmp;
    EmbyChannel &channel = m_channels.at(i);
    PVR_CHANNEL tag;
    memset(&tag, 0, sizeof(PVR_CHANNEL));
    tag.iUniqueId = channel.iUniqueId;
    tag.iChannelNumber = channel.iChannelNumber;
    tag.iSubChannelNumber = channel.iSubChannelNumber;
    tag.iEncryptionSystem = channel.iEncryptionSystem;	
    strncpy(tag.strChannelName, channel.strChannelName.c_str(), sizeof(tag.strChannelName));
    strncpy(tag.strIconPath, channel.strLogoPath.c_str(), sizeof(tag.strIconPath));
    PVR->TransferChannelEntry(handle, &tag);
  }
}

int Emby::RESTGetChannelList(Json::Value& response)
{
  XBMC->Log(LOG_DEBUG, "%s - get channel list entries via REST interface", __FUNCTION__);
  int retval = -1;
  cRest rest;  


  std::string strUrl = m_strBaseUrl + URI_REST_CHANNELS;
  retval = rest.Get(strUrl, "", response,m_strToken);
  if (retval >= 0)
  {
    if (response.type() == Json::objectValue)
    {
      return response["Items"].size();
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
      return -1;
    }
  }
  else
  {
    XBMC->Log(LOG_DEBUG, "Request Channel List failed. Return value: %i\n", retval);
  }

  
  return retval;
}


bool Emby::LoadChannels()
{
  PVR->TriggerChannelUpdate();
  return true;  
}

PVR_ERROR Emby::GetChannelStreamProperties(const PVR_CHANNEL* channel, PVR_NAMED_VALUE* properties, unsigned int* iPropertiesCount)
{
  std:string strUrl;
  for (const auto& EmbyChannel : m_channels)
  {
    if (EmbyChannel.iUniqueId == channel->iUniqueId)
    {
      strUrl = EmbyChannel.strStreamURL;
    }
  }

  if (strUrl.empty()) {
    return PVR_ERROR_FAILED;
  }
  strncpy(properties[0].strName, PVR_STREAM_PROPERTY_STREAMURL, sizeof(properties[0].strName));
  strncpy(properties[0].strValue, strUrl.c_str(), sizeof(properties[0].strValue));

  *iPropertiesCount = 1;

  return PVR_ERROR_NO_ERROR;
}



/************************************************************/
/** Recordings  */

PVR_ERROR Emby::GetRecordings(ADDON_HANDLE handle)
{  
  m_iNumRecordings = 0;
  m_recordings.clear();

  Json::Value data;
  int retval = RESTGetRecordings(data);  
  if (retval > 0) {
    data = data["Items"];
    for (unsigned int index = 0; index < data.size(); ++index)
    {
      EmbyRecording recording;
      Json::Value entry = data[index];

      recording.strRecordingId = entry["Id"].asString();
      recording.strTitle = entry["Name"].asString();
      recording.strDirectory = "";
      recording.startTime = ISO8601ToTime(entry["PremiereDate"].asCString());
      recording.iDuration = entry["RunTimeTicks"].asInt64() / 10000000;
      recording.iLastPlayedPosition = entry["UserData"]["PlaybackPositionTicks"].asInt64() / 10000000;
      
      //std::string params = GetPreviewParams(handle, entry);
      recording.strStreamURL = GetStreamUrl(entry["Id"].asString(),m_strRecordingParameters);
      recording.strIconPath = GetChannelLogo(entry["Id"].asString());
      m_iNumRecordings++;
      m_recordings.push_back(recording);

      XBMC->Log(LOG_DEBUG, "%s loaded Recording entry '%s'", __FUNCTION__, recording.strTitle.c_str());
    }
  }
  
  XBMC->QueueNotification(QUEUE_INFO, "%d recordings loaded.", m_recordings.size());
  
  TransferRecordings(handle);

  return PVR_ERROR_NO_ERROR;
}

void Emby::TransferRecordings(ADDON_HANDLE handle)
{
  for (unsigned int i = 0; i<m_recordings.size(); i++)
  {    
    EmbyRecording &recording = m_recordings.at(i);
    PVR_RECORDING tag;
    memset(&tag, 0, sizeof(PVR_RECORDING));
    strncpy(tag.strRecordingId, recording.strRecordingId.c_str(), sizeof(tag.strRecordingId) -1);
    strncpy(tag.strTitle, recording.strTitle.c_str(), sizeof(tag.strTitle) -1);
    strncpy(tag.strPlotOutline, recording.strPlotOutline.c_str(), sizeof(tag.strPlotOutline) -1);
    strncpy(tag.strPlot, recording.strPlot.c_str(), sizeof(tag.strPlot) -1);
    strncpy(tag.strChannelName, recording.strChannelName.c_str(), sizeof(tag.strChannelName) -1);
    strncpy(tag.strIconPath, recording.strIconPath.c_str(), sizeof(tag.strIconPath) -1);
    strncpy(tag.strDirectory, recording.strDirectory.c_str(), sizeof(tag.strDirectory) -1);
    tag.recordingTime = recording.startTime;
    tag.iDuration = recording.iDuration;

    /* TODO: PVR API 5.0.0: Implement this */
    tag.iChannelUid = PVR_CHANNEL_INVALID_UID;

    /* TODO: PVR API 5.1.0: Implement this */
    tag.channelType = PVR_RECORDING_CHANNEL_TYPE_UNKNOWN;

    PVR->TransferRecordingEntry(handle, &tag);
  }
}

int Emby::RESTGetRecordings(Json::Value& response)
{
  cRest rest;
  std::string strUrl = m_strBaseUrl + URI_REST_RECORDINGS;
  std::string params = StringUtils::Format("?UserId=%s&fields=Path",m_strUserId.c_str());
  int retval = rest.Get(strUrl, params, response, m_strToken);

  if (retval >= 0)
  {
    if (response.type() == Json::objectValue)
    {
      return response["Items"].size();
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::objectValue\n");
      return -1;
    }
  }
  else
  {
    XBMC->Log(LOG_DEBUG, "Request Recordings failed. Return value: %i\n", retval);
  }

  return retval;
}

unsigned int Emby::GetRecordingsAmount() {
  return m_iNumRecordings;
}

PVR_ERROR Emby::GetRecordingStreamProperties(const PVR_RECORDING* recording, PVR_NAMED_VALUE* properties, unsigned int* iPropertiesCount)
{
  std:string strRecordingFile;
  for (const auto& EmbyRec : m_recordings)
  {
    if (strcmp(EmbyRec.strRecordingId.c_str(), recording->strRecordingId)== 0)
    {
      strRecordingFile = EmbyRec.strStreamURL;
    }
  }

  if (strRecordingFile.empty())
    return PVR_ERROR_SERVER_ERROR;

  strncpy(properties[0].strName, PVR_STREAM_PROPERTY_STREAMURL, sizeof(properties[0].strName) - 1);
  strncpy(properties[0].strValue, strRecordingFile.c_str(), sizeof(properties[0].strValue) - 1);
  *iPropertiesCount = 1;
  return PVR_ERROR_NO_ERROR;
}

/************************************************************/
/** Timer */

unsigned int Emby::GetTimersAmount(void)
{  
	return m_timer.size();
}

PVR_ERROR Emby::GetTimers(ADDON_HANDLE handle)
{  
  int y,M,d,h,m;
  float s;
  m_timer.clear();

  Json::Value data;
  int retval = RESTGetTimer(data);  
  if (retval < 0)
  {
    XBMC->Log(LOG_ERROR, "No timer available.");
    return PVR_ERROR_SERVER_ERROR;
  }
  data = data["Items"];

  for (unsigned int index = 0; index < data.size(); ++index)
  {
    EmbyTimer timer;
    Json::Value entry = data[index];

    timer.iId = *((unsigned int*)entry["Id"].asCString());
    timer.strTitle = entry["Name"].asString();
    timer.iChannelId = *((unsigned int*)entry["ChannelId"].asCString());
    timer.iProgramId = *((unsigned int*)entry["ProgramId"].asCString());

    timer.startTime = ISO8601ToTime(entry["StartDate"].asCString());
    timer.endTime = ISO8601ToTime(entry["EndDate"].asCString());
    
    timer.iStartOffset = entry["PrePaddingSeconds"].asInt();
    timer.iEndOffset = entry["PostPaddingSeconds"].asInt();      
    
    std::string strState = entry["Status"].asString();
    if (strState == "Idle" || strState == "Prepared")
    {
      timer.state = PVR_TIMER_STATE_SCHEDULED;
    }
    else if (strState == "Running")
    {
      timer.state = PVR_TIMER_STATE_RECORDING;
    }
    else if (strState == "Done")
    {
      timer.state = PVR_TIMER_STATE_COMPLETED;
    }     
    else
    {
      timer.state = PVR_TIMER_STATE_NEW;  // default
    }
	
    m_timer.push_back(timer);

    XBMC->Log(LOG_DEBUG, "%s loaded Timer entry '%s'", __FUNCTION__, timer.strTitle.c_str());
  }
  
  XBMC->QueueNotification(QUEUE_INFO, "%d timer loaded.", m_timer.size());
  
  TransferTimer(handle);

  return PVR_ERROR_NO_ERROR;
}

void Emby::TransferTimer(ADDON_HANDLE handle)
{
  for (unsigned int i = 0; i<m_timer.size(); i++)
  {
    std::string strTmp;
    EmbyTimer &timer = m_timer.at(i);
    PVR_TIMER tag;
    memset(&tag, 0, sizeof(PVR_TIMER));

    /* TODO: Implement own timer types to get support for the timer features introduced with PVR API 1.9.7 */
    tag.iTimerType = PVR_TIMER_TYPE_NONE;

    tag.iClientIndex = timer.iId;
    tag.iClientChannelUid = timer.iChannelId;
    strncpy(tag.strTitle, timer.strTitle.c_str(), sizeof(tag.strTitle));
    tag.startTime = timer.startTime;
    tag.endTime = timer.endTime;
    tag.state = timer.state;
    tag.strDirectory[0] = '\0';
    tag.iPriority = 0;
    tag.iLifetime = 0;
    tag.iEpgUid = timer.iProgramId;

    PVR->TransferTimerEntry(handle, &tag);
  }
}

int Emby::RESTGetTimer(Json::Value& response)
{
  cRest rest;
  std::string strUrl = m_strBaseUrl + URI_REST_TIMER;
  int retval = rest.Get(strUrl, "", response,m_strToken);

  if (retval >= 0)
  {
    if (response.type() == Json::objectValue)
    {
      return response["Items"].size();
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
      return -1;
    }
  }
  else
  {
    XBMC->Log(LOG_DEBUG, "Request Timer failed. Return value: %i\n", retval);
  }

  return retval;
}


int Emby::RESTAddTimer(std::string id, Json::Value& response)
{	

  // Get empty timer
  std::string strUrl = m_strBaseUrl + "/LiveTV/Timers/Defaults";
  std::string strQueryString = StringUtils::Format("?ProgramId=%s",id.c_str());
  cRest rest;

  int retval = rest.Get(strUrl, strQueryString, response,m_strToken);

  if (retval >= 0)
  {
    if (response.type() == Json::objectValue)
    {
      Json::Value r2;
      // POST to create timer
      Json::FastWriter fastWriter;
      std::string strdata = fastWriter.write(response);
      strUrl = m_strBaseUrl + "/LiveTV/Timers";
      retval = rest.Post(strUrl,strdata,r2,m_strToken);
      return 0;
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
	    return -1;
    }
  }
  else
  {
    XBMC->Log(LOG_DEBUG, "Request Timer failed. Return value: %i\n", retval);
	  return -1;
  }
    
  
  PVR->TriggerTimerUpdate();
  return retval;
}

PVR_ERROR Emby::AddTimer(const PVR_TIMER &timer) 
{
  // Find the Channel
  std:string strChannelId;
  for (const auto& EmbyChannel : m_channels)
  {
    if (EmbyChannel.iUniqueId == (unsigned int)timer.iClientChannelUid)
    {
      strChannelId = EmbyChannel.strEmbyId;
      XBMC->Log(LOG_DEBUG, "AddTimer Found ChannelID: %s.",strChannelId.c_str());
      // Grab the EPG from Emby for this channel
      Json::Value data;  
      int retval = RESTGetEpg(strChannelId, 0, 0, data);
      if (retval < 0)
      {
        XBMC->Log(LOG_ERROR, "AddTimer Could not retrieve EPG.");
        return PVR_ERROR_SERVER_ERROR;
      }
      data = data["Items"];
      // Match EPG Entry
      for (unsigned int i = 0; i < data.size(); ++i)
      {
        if (*((unsigned int*)data[i]["Id"].asCString()) == timer.iEpgUid) {
          // Add Timer for that entry
          int retval = RESTAddTimer(data[i]["Id"].asString(), data);
          if (retval == 0) {
            PVR->TriggerTimerUpdate();
            return PVR_ERROR_NO_ERROR;
          }
          XBMC->Log(LOG_ERROR, "AddTimer Failed.");
          return PVR_ERROR_SERVER_ERROR;
        }
      }
    }
  }
  
  XBMC->Log(LOG_ERROR, "AddTimer Only Supports Recordings based on EPG Entry. EPG Entry not found.");
  return PVR_ERROR_SERVER_ERROR;
}


/************************************************************/
/** EPG  */

PVR_ERROR Emby::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  XBMC->Log(LOG_DEBUG, "%s - Channel: %s\n", __FUNCTION__, channel.strChannelName);
  int y,M,d,h,m;
  float s;

  Json::Value data;
  for (vector<EmbyChannel>::iterator myChannel = m_channels.begin(); myChannel < m_channels.end(); ++myChannel)
  {
    if (myChannel->iUniqueId != channel.iUniqueId) continue;
	  if (!GetEPG(myChannel->strEmbyId, iStart, iEnd, data)) continue;
    Json::Value entries = data["Items"];
    if (entries.size() <= 0) continue;

    int iChannelId = myChannel->iUniqueId;
    EPG_TAG epg;
    for (unsigned int i = 0; i < entries.size(); ++i)
    {

      Json::Value entry = entries[i];
      memset(&epg, 0, sizeof(EPG_TAG));
      
      epg.iUniqueBroadcastId = *((unsigned int*)entry["Id"].asCString());
      
      epg.strTitle = entry["Name"].asCString();
      epg.iUniqueChannelId = iChannelId;

      epg.startTime = ISO8601ToTime(entry["StartDate"].asCString());
      epg.endTime = ISO8601ToTime(entry["EndDate"].asCString());

      //epg.startTime = static_cast<time_t>(entry["StartTime"].asDouble() / 1000);
      //epg.endTime = static_cast<time_t>(entry["EndTime"].asDouble() / 1000);
      if (entry["Overview"]!= Json::nullValue)
        epg.strPlot = entry["Overview"].asCString();
      epg.strPlotOutline = NULL;
      epg.strOriginalTitle = NULL; // unused
      epg.strCast = NULL; // unused
      epg.strDirector = NULL; // unused
      epg.strWriter = NULL; // unused
      if (entry["ProductionYear"]!= Json::nullValue) 
        epg.iYear = entry["ProductionYear"].asInt();
      epg.strIMDBNumber = NULL; // unused
      epg.strIconPath = ""; // unused
      epg.iGenreType = 0; // unused
      epg.iGenreSubType = 0; // unused
      epg.strGenreDescription = "";
      epg.firstAired = 0; // unused
      epg.iParentalRating = 0; // unused
      epg.iStarRating = 0; // unused
      epg.bNotify = false;
      epg.iSeriesNumber = 0; // unused
      epg.iEpisodeNumber = 0; // unused
      epg.iEpisodePartNumber = 0; // unused
      if (entry["EpisodeTitle"]!= Json::nullValue)
        epg.strEpisodeName = entry["EpisodeTitle"].asCString();
      epg.iFlags = EPG_TAG_FLAG_UNDEFINED;

      PVR->TransferEpgEntry(handle, &epg);
    }

	  return PVR_ERROR_NO_ERROR;
  }
    
  return PVR_ERROR_SERVER_ERROR;
}

bool Emby::GetEPG(std::string id, time_t iStart, time_t iEnd, Json::Value& data)
{   
  int retval = RESTGetEpg(id, iStart, iEnd, data);
  if (retval < 0)
  {
    XBMC->Log(LOG_ERROR, "No EPG data retrieved.");
    return false;
  }
  
  XBMC->Log(LOG_NOTICE, "EPG Loaded.");
  return true;
}


int Emby::RESTGetEpg(std::string id, time_t iStart, time_t iEnd, Json::Value& response)
{
  std::string strParams;
  //strParams= StringUtils::Format("?ids=%d&extended=1&start=%d&end=%d", id, iStart * 1000, iEnd * 1000);
  strParams= StringUtils::Format("?ChannelIds=%s&fields=Overview", id.c_str());
  
  cRest rest;
  std::string strUrl = m_strBaseUrl + URI_REST_EPG;
  int retval = rest.Get(strUrl, strParams, response,m_strToken);
  if (retval >= 0)
  {
    if (response.type() == Json::objectValue)
    {
      return response["Items"].size();
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
      return -1;
    }
  }
  else
  {
    XBMC->Log(LOG_DEBUG, "Request EPG failed. Return value: %i\n", retval);
  }

  return retval;
}

std::string Emby::GetStreamUrl(std::string id,std::string params)
{
  std::string strTmp;
  strTmp = StringUtils::Format("%s/Videos/%s/stream%s", m_strBaseUrl.c_str(), id.c_str(), params.c_str());
  return strTmp;
}

std::string Emby::GetChannelLogo(std::string params)
{
  std::string strTmp;
  strTmp = StringUtils::Format("%s/Items/%s/Images/Primary", m_strBaseUrl.c_str(), params.c_str());
  return strTmp;
}


bool Emby::IsConnected()
{
  return m_bIsConnected;
}


const char* Emby::GetBackendName()
{
  return m_strBackendName.c_str();
}

const char* Emby::GetBackendVersion()
{
  return m_strBackendVersion.c_str();
}

bool Emby::GetChannel(const PVR_CHANNEL &channel, EmbyChannel &myChannel)
{
  for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
  {    
    EmbyChannel &thisChannel = m_channels.at(iChannelPtr);
    if (thisChannel.iUniqueId == channel.iUniqueId)
    {
      myChannel.iUniqueId = thisChannel.iUniqueId;
      myChannel.bRadio = thisChannel.bRadio;
      myChannel.iChannelNumber = thisChannel.iChannelNumber;
      myChannel.iEncryptionSystem = thisChannel.iEncryptionSystem;
      myChannel.strChannelName = thisChannel.strChannelName;
      myChannel.strLogoPath = thisChannel.strLogoPath;
      myChannel.strStreamURL = thisChannel.strStreamURL;
      return true;
    }
  }

  return false;
}

unsigned int Emby::GetChannelsAmount()
{
  return m_channels.size();
}


int Emby::ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  return 0;
}

long long Emby::SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */)
{
  return 0;
}

long long Emby::PositionLiveStream(void)
{
  return 0;
}

long long Emby::LengthLiveStream(void)
{
  return 0;
}

/* ################ misc ################ */

/*
* \brief Get a channel list from Emby Device via REST interface
* \param id The channel list id
*/

time_t Emby::ISO8601ToTime(const char* date) {
  int y,M,d,h,m;
  float s;
  sscanf(date, "%d-%d-%dT%d:%d:%fZ", &y, &M, &d, &h, &m, &s);
  tm time;
  time.tm_year = y - 1900; // Year since 1900
  time.tm_mon = M - 1;     // 0-11
  time.tm_mday = d;        // 1-31
  time.tm_hour = h;        // 0-23
  time.tm_min = m;         // 0-59
  time.tm_sec = (int)s;    // 0-61 (0-60 in C++11)
  return timegm(&time);
}



