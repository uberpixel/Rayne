//
//  RNEOSLobbyManager.cpp
//  Rayne-EOS
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNEOSLobbyManager.h"
#include "RNEOSWorld.h"

#include "eos_common.h"
#include "eos_platform_prereqs.h"
#include "eos_sdk.h"

#include "eos_lobby.h"
#include "eos_lobby_types.h"
#include "eos_rtc.h"
#include "eos_rtc_audio.h"
#include "eos_rtc_audio_types.h"

namespace RN
{
	RNDefineMeta(EOSLobbyInfo, Object)
	RNDefineMeta(EOSLobbySearchParameter, Object)
	RNDefineMeta(EOSLobbySearchParameterString, EOSLobbySearchParameter)
	RNDefineMeta(EOSLobbyManager, Object)

	EOSLobbyInfo::EOSLobbyInfo() :
		lobbyName(nullptr), lobbyLevel(nullptr), lobbyVersion(nullptr), maximumPlayerCount(0), currentPlayerCount(0), lobbyHandle(nullptr), ownerHandle(nullptr), createTimestamp(0)
	{
	}

	EOSLobbyInfo::~EOSLobbyInfo()
	{
		SafeRelease(lobbyName);
		SafeRelease(lobbyLevel);
		SafeRelease(lobbyVersion);
		EOS_LobbyDetails_Release(lobbyHandle);
	}

	EOSLobbySearchParameter::EOSLobbySearchParameter(String *name, Comparator comparator) :
		_name(SafeRetain(name)), _comparator(comparator)
	{
	}

	EOSLobbySearchParameter::~EOSLobbySearchParameter()
	{
		SafeRelease(_name);
	}

	EOSLobbySearchParameterString::EOSLobbySearchParameterString(String *name, String *content, Comparator comparator) :
		EOSLobbySearchParameter(name, comparator), _content(SafeRetain(content))
	{
	}

	EOSLobbySearchParameterString::~EOSLobbySearchParameterString()
	{
		SafeRelease(_content);
	}

	const String *EOSLobbyInfo::GetDescription() const
	{
		return RNSTR("<" << GetClass()->GetFullname() << ":" << (void *)this << ">\n{\n	lobbyName: " << lobbyName << ",\n	lobbyLevel: " << lobbyLevel << ",\n	lobbyVersion: " << lobbyVersion << ",\n	maximumPlayerCount: " << maximumPlayerCount << ",\n	currentPlayerCount: " << currentPlayerCount << "\n}");
	}

	EOSLobbyManager::EOSLobbyManager(EOSWorld *world) :
		_createLobbyName(nullptr), _createLobbyVersion(nullptr), _isJoiningLobby(false), _didJoinLobbyCallback(nullptr), _isConnectedToLobby(false), _connectedLobbyID(nullptr), _isConnectedLobbyOwner(false), _isVoiceEnabled(false), _isVoiceUnmixed(true), _isLocalPlayerMuted(false), _audioReceivedCallback(nullptr), _audioBeforeSendCallback(nullptr), _currentAudioBeforeRenderNotificationID(0), _currentAudioBeforeSendNotificationID(0)
	{
		_lobbyInterfaceHandle = EOS_Platform_GetLobbyInterface(world->GetPlatformHandle());

		_rtcInterfaceHandle = EOS_Platform_GetRTCInterface(world->GetPlatformHandle());
		_rtcAudioInterfaceHandle = EOS_RTC_GetAudioInterface(_rtcInterfaceHandle);

		EOS_Lobby_AddNotifyLobbyMemberStatusReceivedOptions statusOptions = {};
		statusOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYMEMBERSTATUSRECEIVED_API_LATEST;
		_memberStatusReceivedNotificationID = EOS_Lobby_AddNotifyLobbyMemberStatusReceived(_lobbyInterfaceHandle, &statusOptions, this, LobbyOnMemberStatusReceived);
	}

	EOSLobbyManager::~EOSLobbyManager()
	{
		ResetLobbySearchCallback();
		SafeRelease(_connectedLobbyID);
		EOS_Lobby_RemoveNotifyLobbyMemberStatusReceived(_lobbyInterfaceHandle, _memberStatusReceivedNotificationID);
	}

	void EOSLobbyManager::SetGlobalAudioOptions(bool voiceEnabled, bool unmixed, std::function<void(RN::String *eosUserID, RN::uint32 sampleRate, RN::uint32 channels, RN::uint32 framesCount, RN::int16 *frames)> audioReceivedCallback, std::function<void(RN::uint32 sampleRate, RN::uint32 channels, RN::uint32 framesCount, RN::int16 *frames)> audioBeforeSendCallback)
	{
		_audioReceivedCallback = audioReceivedCallback;
		_audioBeforeSendCallback = audioBeforeSendCallback;
		_isVoiceEnabled = voiceEnabled;
		_isVoiceUnmixed = unmixed;
	}

	void EOSLobbyManager::SetLocalPlayerMuted(bool mute)
	{
		if(_isConnectedToLobby && mute != _isLocalPlayerMuted)
		{
			EOS_Lobby_GetRTCRoomNameOptions roomNameOptions = {};
			roomNameOptions.ApiVersion = EOS_LOBBY_GETRTCROOMNAME_API_LATEST;
			roomNameOptions.LobbyId = _connectedLobbyID->GetUTF8String();
			roomNameOptions.LocalUserId = EOSWorld::GetInstance()->GetUserID();
			char roomNameBuffer[512];
			RN::uint32 roomNameLength = 512;
			if(EOS_Lobby_GetRTCRoomName(_lobbyInterfaceHandle, &roomNameOptions, roomNameBuffer, &roomNameLength) == EOS_EResult::EOS_Success)
			{
				EOS_RTCAudio_UpdateSendingOptions sendingOptions = {};
				sendingOptions.ApiVersion = EOS_RTCAUDIO_UPDATESENDING_API_LATEST;
				sendingOptions.RoomName = roomNameBuffer;
				sendingOptions.LocalUserId = EOSWorld::GetInstance()->GetUserID();
				sendingOptions.AudioStatus = mute ? EOS_ERTCAudioStatus::EOS_RTCAS_Disabled : EOS_ERTCAudioStatus::EOS_RTCAS_Enabled;
				EOS_RTCAudio_UpdateSending(_rtcAudioInterfaceHandle, &sendingOptions, this, LobbyAudioOnUpdateSendingCallback);
			}
		}

		_isLocalPlayerMuted = mute;
	}

	void EOSLobbyManager::CreateLobby(int64 createLobbyTimestamp, String *lobbyName, String *lobbyLevel, uint8 maxUsers, std::function<void(EOSResult)> callback, String *lobbyVersion, bool hasPassword, const String *lobbyIDOverride)
	{
		if(EOSWorld::GetInstance()->GetLoginState() != EOSWorld::LoginStateIsLoggedIn)
		{
			if(callback) callback(EOSResult::NotLoggedIn);
			return;
		}
		if(_isJoiningLobby || _isConnectedToLobby)
		{
			return;
		}

		if(!EOSWorld::GetInstance()->GetHasNetworkConnection())
		{
			if(callback) callback(EOSResult::NoConnection);
			return;
		}

		RN_ASSERT(!lobbyIDOverride || (lobbyIDOverride->GetLength() <= EOS_LOBBY_MAX_LOBBYIDOVERRIDE_LENGTH && lobbyIDOverride->GetLength() >= EOS_LOBBY_MIN_LOBBYIDOVERRIDE_LENGTH), "Lobby ID override has an unsupported number of characters");

		_isJoiningLobby = true;
		_didJoinLobbyCallback = callback;
		_createLobbyName = SafeRetain(lobbyName);
		_createLobbyLevel = SafeRetain(lobbyLevel);
		_createLobbyVersion = SafeRetain(lobbyVersion);
		_createLobbyTimestamp = createLobbyTimestamp;
		_createLobbyHasPassword = hasPassword;

		EOS_Lobby_CreateLobbyOptions options = {};
		options.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
		options.LocalUserId = EOSWorld::GetInstance()->GetUserID();
		options.MaxLobbyMembers = maxUsers;
		//Don't include lobbies with max 1 users in any search results, nobody can join them anyway
		options.PermissionLevel = maxUsers == 1 ? EOS_ELobbyPermissionLevel::EOS_LPL_INVITEONLY : EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED;
		options.bPresenceEnabled = false;
		options.bDisableHostMigration = false; //Allow host migration
		options.bEnableJoinById = true;
		options.BucketId = "Server"; //Top-level filtering criteria, called the Bucket ID, which is specific to your game; often formatted like "GameMode:Region:MapName"
		if(lobbyIDOverride) options.LobbyId = lobbyIDOverride->GetUTF8String();

		EOS_Lobby_LocalRTCOptions localRTCOptions = {0};
		if(_isVoiceEnabled)
		{
			options.bEnableRTCRoom = true;

			localRTCOptions.ApiVersion = EOS_LOBBY_LOCALRTCOPTIONS_API_LATEST;
			localRTCOptions.bLocalAudioDeviceInputStartsMuted = _isLocalPlayerMuted;
			localRTCOptions.bUseManualAudioInput = false;
			localRTCOptions.bUseManualAudioOutput = false;

			RNInfo("localRTCOptions.ApiVersion: " << localRTCOptions.ApiVersion);

			if(_audioReceivedCallback)
			{
				localRTCOptions.bUseManualAudioOutput = true;
			}
			options.LocalRTCOptions = &localRTCOptions;
		}

		EOS_Lobby_CreateLobby(_lobbyInterfaceHandle, &options, this, LobbyOnCreateCallback);
	}

	void EOSLobbyManager::SearchLobby(bool includePrivate, bool includePublic, uint32 maxResults, std::function<void(EOSResult, RN::Array *)> callback, const RN::String *lobbyID, RN::Array *searchFilter)
	{
		if(EOSWorld::GetInstance()->GetLoginState() != EOSWorld::LoginStateIsLoggedIn)
		{
			if(callback) callback(EOSResult::NotLoggedIn, nullptr);
			return;
		}

		if(!EOSWorld::GetInstance()->GetHasNetworkConnection())
		{
			if(callback) callback(EOSResult::NoConnection, nullptr);
			return;
		}

		EOSLobbySearch *searchData = new EOSLobbySearch();
		searchData->callback = callback;

		EOS_Lobby_CreateLobbySearchOptions searchOptions = {0};
		searchOptions.ApiVersion = EOS_LOBBY_CREATELOBBYSEARCH_API_LATEST;
		searchOptions.MaxResults = std::min(maxResults, static_cast<uint32>(EOS_LOBBY_MAX_SEARCH_RESULTS));

		if(EOS_Lobby_CreateLobbySearch(_lobbyInterfaceHandle, &searchOptions, &searchData->handle) != EOS_EResult::EOS_Success)
		{
			RNDebug("Failed creating EOS Lobby search handle");
			if(callback) callback(EOSResult::Other, nullptr);
			return;
		}

		if(lobbyID)
		{
			EOS_LobbySearch_SetLobbyIdOptions lobbyIDOptions = {0};
			lobbyIDOptions.ApiVersion = EOS_LOBBYSEARCH_SETLOBBYID_API_LATEST;
			lobbyIDOptions.LobbyId = lobbyID->GetUTF8String();
			EOS_LobbySearch_SetLobbyId(searchData->handle, &lobbyIDOptions);
		}
		else //Can't set anything else if setting a lobby id!
		{
			EOS_Lobby_AttributeData timestampAttributeData = {0};
			timestampAttributeData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
			timestampAttributeData.ValueType = EOS_EAttributeType::EOS_AT_INT64;
			timestampAttributeData.Key = "timestamp";
			timestampAttributeData.Value.AsInt64 = std::numeric_limits<int64>::max();

			EOS_LobbySearch_SetParameterOptions timestampSearchParameterOptions = {0};
			timestampSearchParameterOptions.ApiVersion = EOS_LOBBYSEARCH_SETPARAMETER_API_LATEST;
			timestampSearchParameterOptions.ComparisonOp = EOS_EComparisonOp::EOS_CO_LESSTHANOREQUAL;
			timestampSearchParameterOptions.Parameter = &timestampAttributeData;

			EOS_LobbySearch_SetParameter(searchData->handle, &timestampSearchParameterOptions);

			if(includePublic != includePrivate)
			{
				EOS_Lobby_AttributeData attributeData = {0};
				attributeData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
				attributeData.ValueType = EOS_EAttributeType::EOS_AT_BOOLEAN;
				attributeData.Key = "hasPassword";
				attributeData.Value.AsBool = includePrivate;

				EOS_LobbySearch_SetParameterOptions searchParameterOptions = {0};
				searchParameterOptions.ApiVersion = EOS_LOBBYSEARCH_SETPARAMETER_API_LATEST;
				searchParameterOptions.ComparisonOp = EOS_EComparisonOp::EOS_CO_EQUAL;
				searchParameterOptions.Parameter = &attributeData;

				EOS_LobbySearch_SetParameter(searchData->handle, &searchParameterOptions);
			}

			if(searchFilter && searchFilter->GetCount() > 0)
			{
				searchFilter->Enumerate<EOSLobbySearchParameter>([&](EOSLobbySearchParameter *param, size_t index, bool &stop) {
					EOSLobbySearchParameterString *paramString = param->Downcast<EOSLobbySearchParameterString>();
					if(paramString)
					{
						EOS_Lobby_AttributeData attributeData = {0};
						attributeData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
						attributeData.ValueType = EOS_EAttributeType::EOS_AT_STRING;
						attributeData.Key = paramString->_name->GetUTF8String();
						attributeData.Value.AsUtf8 = paramString->_content->GetUTF8String();

						EOS_LobbySearch_SetParameterOptions searchParameterOptions = {0};
						searchParameterOptions.ApiVersion = EOS_LOBBYSEARCH_SETPARAMETER_API_LATEST;
						searchParameterOptions.Parameter = &attributeData;

						if(paramString->_comparator == EOSLobbySearchParameter::ComparatorEqual)
						{
							searchParameterOptions.ComparisonOp = EOS_EComparisonOp::EOS_CO_EQUAL;
						}
						else if(paramString->_comparator == EOSLobbySearchParameter::ComparatorNotEqual)
						{
							searchParameterOptions.ComparisonOp = EOS_EComparisonOp::EOS_CO_NOTEQUAL;
						}
						else if(paramString->_comparator == EOSLobbySearchParameter::ComparatorStringAnyOf)
						{
							searchParameterOptions.ComparisonOp = EOS_EComparisonOp::EOS_CO_ANYOF;
						}
						else if(paramString->_comparator == EOSLobbySearchParameter::ComparatorStringNotAnyOf)
						{
							searchParameterOptions.ComparisonOp = EOS_EComparisonOp::EOS_CO_NOTANYOF;
						}
						else
						{
							RN_ASSERT(false, "Unsupported comparator!");
						}

						EOS_LobbySearch_SetParameter(searchData->handle, &searchParameterOptions);
					}
				});
			}
		}

		EOS_LobbySearch_FindOptions findOptions = {0};
		findOptions.ApiVersion = EOS_LOBBYSEARCH_FIND_API_LATEST;
		findOptions.LocalUserId = EOSWorld::GetInstance()->GetUserID();

		//Clear up previous lobby searches
		for(int i = _lobbySearches.size() - 1; i >= 0; i--)
		{
			if(!_lobbySearches[i]->handle)
			{
				delete _lobbySearches[i];
				_lobbySearches.erase(_lobbySearches.begin() + i);
			}
		}
		_lobbySearches.push_back(searchData);

		RNDebug("Start searching lobbies");
		EOS_LobbySearch_Find(searchData->handle, &findOptions, searchData, LobbyOnSearchCallback);
	}

	void EOSLobbyManager::JoinLobby(EOSLobbyInfo *lobbyInfo, std::function<void(EOSResult)> callback)
	{
		if(_isJoiningLobby || _isConnectedToLobby) return;
		if(EOSWorld::GetInstance()->GetLoginState() != EOSWorld::LoginStateIsLoggedIn)
		{
			if(callback) callback(EOSResult::NotLoggedIn);
			return;
		}

		if(!EOSWorld::GetInstance()->GetHasNetworkConnection())
		{
			if(callback) callback(EOSResult::NoConnection);
			return;
		}

		_isJoiningLobby = true;
		_didJoinLobbyCallback = callback;

		EOS_Lobby_JoinLobbyOptions joinOptions = {0};
		joinOptions.ApiVersion = EOS_LOBBY_JOINLOBBY_API_LATEST;
		joinOptions.LocalUserId = EOSWorld::GetInstance()->GetUserID();
		joinOptions.LobbyDetailsHandle = lobbyInfo->lobbyHandle;
		joinOptions.bPresenceEnabled = false;

		EOS_Lobby_LocalRTCOptions localRTCOptions = {0};
		if(_isVoiceEnabled)
		{
			localRTCOptions.ApiVersion = EOS_LOBBY_LOCALRTCOPTIONS_API_LATEST;
			localRTCOptions.bLocalAudioDeviceInputStartsMuted = _isLocalPlayerMuted;
			localRTCOptions.bUseManualAudioInput = false;
			localRTCOptions.bUseManualAudioOutput = false;

			if(_audioReceivedCallback)
			{
				localRTCOptions.bUseManualAudioOutput = true;
			}
			joinOptions.LocalRTCOptions = &localRTCOptions;
		}

		EOS_Lobby_JoinLobby(_lobbyInterfaceHandle, &joinOptions, this, LobbyOnJoinCallback);
	}

	void EOSLobbyManager::RetrievePeers()
	{
		RN_ASSERT(_isConnectedToLobby, "Cannot retrieve peers: not connected to lobby.");

		EOS_LobbyDetails_GetMemberCountOptions getMemberCountOptions {};
		getMemberCountOptions.ApiVersion = EOS_LOBBYDETAILS_GETMEMBERCOUNT_API_LATEST;
		uint32_t memberCount = EOS_LobbyDetails_GetMemberCount(_lobbyDetails, &getMemberCountOptions);
		_remotePeers.clear();

		for(uint32_t i = 0; i < memberCount; ++i)
		{
			EOS_LobbyDetails_GetMemberByIndexOptions getMemberOptions {};
			getMemberOptions.ApiVersion = EOS_LOBBYDETAILS_GETMEMBERBYINDEX_API_LATEST;
			getMemberOptions.MemberIndex = i;
			EOS_ProductUserId memberId = EOS_LobbyDetails_GetMemberByIndex(_lobbyDetails, &getMemberOptions);

			if(memberId != EOSWorld::GetInstance()->GetUserID())
			{
				_remotePeers.push_back(memberId);
			}
		}
	}
	void EOSLobbyManager::AddRemotePeer(EOS_ProductUserId peerID)
	{
		_remotePeers.push_back(peerID);
	}
	void EOSLobbyManager::RemoveRemotePeer(EOS_ProductUserId peerID)
	{
		_remotePeers.erase(std::remove(_remotePeers.begin(), _remotePeers.end(), peerID), _remotePeers.end());
	}

	void EOSLobbyManager::LeaveCurrentLobby()
	{
		if(!_isConnectedToLobby) return;

		if(_isConnectedLobbyOwner && _remotePeers.empty())
		{
			EOS_Lobby_DestroyLobbyOptions destroyOptions;
			destroyOptions.ApiVersion = EOS_LOBBY_DESTROYLOBBY_API_LATEST;
			destroyOptions.LocalUserId = EOSWorld::GetInstance()->GetUserID();
			destroyOptions.LobbyId = _connectedLobbyID->GetUTF8String();
			EOS_Lobby_DestroyLobby(_lobbyInterfaceHandle, &destroyOptions, this, LobbyOnDestroyCallback);
		}
		else
		{
			EOS_Lobby_LeaveLobbyOptions leaveOptions = {0};
			leaveOptions.ApiVersion = EOS_LOBBY_LEAVELOBBY_API_LATEST;
			leaveOptions.LocalUserId = EOSWorld::GetInstance()->GetUserID();
			leaveOptions.LobbyId = _connectedLobbyID->GetUTF8String();
			EOS_Lobby_LeaveLobby(_lobbyInterfaceHandle, &leaveOptions, this, LobbyOnLeaveCallback);
		}

		if(_lobbyDetails)
		{
			EOS_LobbyDetails_Release(_lobbyDetails);
			_lobbyDetails = nullptr;
		}

		_remotePeers.clear();
	}

	void EOSLobbyManager::KickFromCurrentLobby(EOS_ProductUserId userHandle)
	{
		EOS_Lobby_KickMemberOptions kickOptions = {0};
		kickOptions.ApiVersion = EOS_LOBBY_KICKMEMBER_API_LATEST;
		kickOptions.LocalUserId = EOSWorld::GetInstance()->GetUserID();
		kickOptions.LobbyId = _connectedLobbyID->GetUTF8String();
		kickOptions.TargetUserId = userHandle;
		EOS_Lobby_KickMember(_lobbyInterfaceHandle, &kickOptions, this, LobbyOnKickMemberCallback);
	}

	void EOSLobbyManager::SetCurrentLobbyAttributes(Dictionary *attributes)
	{
		//Can only edit a lobby if connected to it and the owner
		if(!_isConnectedToLobby || !_isConnectedLobbyOwner) return;

		EOS_Lobby_UpdateLobbyModificationOptions modificationOptions = {0};
		modificationOptions.ApiVersion = EOS_LOBBY_UPDATELOBBYMODIFICATION_API_LATEST;
		modificationOptions.LobbyId = _connectedLobbyID->GetUTF8String();
		modificationOptions.LocalUserId = EOSWorld::GetInstance()->GetUserID();

		EOS_HLobbyModification modificationHandle;
		if(EOS_Lobby_UpdateLobbyModification(_lobbyInterfaceHandle, &modificationOptions, &modificationHandle) != EOS_EResult::EOS_Success)
		{
			RNDebug("Failed creating EOS Lobby modification handle");
			return;
		}

		attributes->Enumerate([&](Object *object, const Object *key, bool &stop) {
			const String *keyString = key->Downcast<String>();

			EOS_Lobby_AttributeData attributeData = {0};
			attributeData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
			attributeData.Key = keyString->GetUTF8String();

			if(object->IsKindOfClass(String::GetMetaClass()))
			{
				String *valueString = object->Downcast<String>();
				attributeData.ValueType = EOS_EAttributeType::EOS_AT_STRING;
				attributeData.Value.AsUtf8 = valueString->GetUTF8String();
			}
			else if(object->IsKindOfClass(Number::GetMetaClass()))
			{
				Number *valueNumber = object->Downcast<Number>();
				switch(valueNumber->GetType())
				{
					case Number::Type::Boolean:
					{
						attributeData.ValueType = EOS_EAttributeType::EOS_AT_BOOLEAN;
						attributeData.Value.AsBool = valueNumber->GetBoolValue();
						break;
					}

					case Number::Type::Float32:
					case Number::Type::Float64:
					{
						attributeData.ValueType = EOS_EAttributeType::EOS_AT_DOUBLE;
						attributeData.Value.AsDouble = valueNumber->GetDoubleValue();
						break;
					}

					case Number::Type::Int8:
					case Number::Type::Int16:
					case Number::Type::Int32:
					case Number::Type::Int64:
					{
						attributeData.ValueType = EOS_EAttributeType::EOS_AT_INT64;
						attributeData.Value.AsInt64 = valueNumber->GetInt64Value();
						break;
					}

					default:
					{
						RN_ASSERT(false, "Unsupported attribute type! (needs to be intN, float or double)");
					}
				}
			}
			else
			{
				RN_ASSERT(false, "Unsupported attribute type! (needs to be String or Number)");
			}

			EOS_LobbyModification_AddAttributeOptions attributeOptions = {0};
			attributeOptions.ApiVersion = EOS_LOBBYMODIFICATION_ADDATTRIBUTE_API_LATEST;
			attributeOptions.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC;
			attributeOptions.Attribute = &attributeData;

			EOS_LobbyModification_AddAttribute(modificationHandle, &attributeOptions);
		});

		EOS_Lobby_UpdateLobbyOptions updateLobbyOptions = {0};
		updateLobbyOptions.ApiVersion = EOS_LOBBY_UPDATELOBBY_API_LATEST;
		updateLobbyOptions.LobbyModificationHandle = modificationHandle;

		EOS_Lobby_UpdateLobby(_lobbyInterfaceHandle, &updateLobbyOptions, this, LobbyOnUpdateCallback);
	}

	void EOSLobbyManager::ResetLobbySearchCallback()
	{
		for(auto search : _lobbySearches)
		{
			if(search->handle) EOS_LobbySearch_Release(search->handle);
			delete search;
		}
		_lobbySearches.clear();
	}

	void EOSLobbyManager::LobbyOnCreateCallback(const EOS_Lobby_CreateLobbyCallbackInfo *Data)
	{
		EOSLobbyManager *lobbyManager = static_cast<EOSLobbyManager *>(Data->ClientData);

		if(Data->ResultCode == EOS_EResult::EOS_Success)
		{
			RNDebug("Lobby successfully created with ID: " << Data->LobbyId);

			lobbyManager->_isConnectedToLobby = true;
			lobbyManager->_connectedLobbyID = new String(Data->LobbyId);
			lobbyManager->_isConnectedLobbyOwner = true;

			EOS_Lobby_CopyLobbyDetailsHandleOptions copyOptions;
			copyOptions.ApiVersion = EOS_LOBBY_COPYLOBBYDETAILSHANDLE_API_LATEST;
			copyOptions.LocalUserId = EOSWorld::GetInstance()->GetUserID();
			copyOptions.LobbyId = Data->LobbyId;
			EOS_EResult copyDetailsResult = EOS_Lobby_CopyLobbyDetailsHandle(lobbyManager->_lobbyInterfaceHandle, &copyOptions, &lobbyManager->_lobbyDetails);

			if(lobbyManager->_isVoiceEnabled && (lobbyManager->_audioReceivedCallback || lobbyManager->_audioBeforeSendCallback))
			{
				EOS_Lobby_GetRTCRoomNameOptions roomNameOptions = {};
				roomNameOptions.ApiVersion = EOS_LOBBY_GETRTCROOMNAME_API_LATEST;
				roomNameOptions.LobbyId = lobbyManager->_connectedLobbyID->GetUTF8String();
				roomNameOptions.LocalUserId = EOSWorld::GetInstance()->GetUserID();
				char roomNameBuffer[512];
				RN::uint32 roomNameLength = 512;
				if(EOS_Lobby_GetRTCRoomName(lobbyManager->_lobbyInterfaceHandle, &roomNameOptions, roomNameBuffer, &roomNameLength) == EOS_EResult::EOS_Success)
				{
					if(lobbyManager->_audioReceivedCallback)
					{
						EOS_RTCAudio_AddNotifyAudioBeforeRenderOptions options = {};
						options.ApiVersion = EOS_RTCAUDIO_ADDNOTIFYAUDIOBEFORERENDER_API_LATEST;
						options.LocalUserId = EOSWorld::GetInstance()->GetUserID();
						options.bUnmixedAudio = lobbyManager->_isVoiceUnmixed;
						options.RoomName = roomNameBuffer;

						lobbyManager->_currentAudioBeforeRenderNotificationID = EOS_RTCAudio_AddNotifyAudioBeforeRender(lobbyManager->_rtcAudioInterfaceHandle, &options, lobbyManager, LobbyAudioOnBeforeRenderCallback);
					}

					if(lobbyManager->_audioBeforeSendCallback)
					{
						EOS_RTCAudio_AddNotifyAudioBeforeSendOptions options = {};
						options.ApiVersion = EOS_RTCAUDIO_ADDNOTIFYAUDIOBEFORESEND_API_LATEST;
						options.LocalUserId = EOSWorld::GetInstance()->GetUserID();
						options.RoomName = roomNameBuffer;

						lobbyManager->_currentAudioBeforeSendNotificationID = EOS_RTCAudio_AddNotifyAudioBeforeSend(lobbyManager->_rtcAudioInterfaceHandle, &options, lobbyManager, LobbyAudioOnBeforeSendCallback);
					}
				}
			}

			if(lobbyManager->_isVoiceEnabled)
			{
				EOS_RTCAudio_SetInputDeviceSettingsOptions audioInputSettings = {};
				audioInputSettings.ApiVersion = EOS_RTCAUDIO_SETINPUTDEVICESETTINGS_API_LATEST;
				audioInputSettings.LocalUserId = EOSWorld::GetInstance()->GetUserID();
				audioInputSettings.bPlatformAEC = EOS_TRUE;
				EOS_RTCAudio_SetInputDeviceSettings(lobbyManager->_rtcAudioInterfaceHandle, &audioInputSettings, nullptr, nullptr);
			}

			EOS_Lobby_UpdateLobbyModificationOptions modificationOptions = {0};
			modificationOptions.ApiVersion = EOS_LOBBY_UPDATELOBBYMODIFICATION_API_LATEST;
			modificationOptions.LobbyId = Data->LobbyId;
			modificationOptions.LocalUserId = EOSWorld::GetInstance()->GetUserID();

			EOS_HLobbyModification modificationHandle;
			if(EOS_Lobby_UpdateLobbyModification(lobbyManager->_lobbyInterfaceHandle, &modificationOptions, &modificationHandle) != EOS_EResult::EOS_Success)
			{
				RNDebug("Failed creating EOS Lobby modification handle");
				return;
			}

			EOS_Lobby_AttributeData searchableAttributeData = {0};
			searchableAttributeData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
			searchableAttributeData.ValueType = EOS_EAttributeType::EOS_AT_BOOLEAN;
			searchableAttributeData.Key = "hasPassword";
			searchableAttributeData.Value.AsBool = lobbyManager->_createLobbyHasPassword;

			EOS_LobbyModification_AddAttributeOptions attributeOptions = {0};
			attributeOptions.ApiVersion = EOS_LOBBYMODIFICATION_ADDATTRIBUTE_API_LATEST;
			attributeOptions.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC;
			attributeOptions.Attribute = &searchableAttributeData;

			EOS_LobbyModification_AddAttribute(modificationHandle, &attributeOptions);

			EOS_Lobby_AttributeData timestampAttributeData = {0};
			timestampAttributeData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
			timestampAttributeData.ValueType = EOS_EAttributeType::EOS_AT_INT64;
			timestampAttributeData.Key = "timestamp";
			timestampAttributeData.Value.AsInt64 = lobbyManager->_createLobbyTimestamp;
			attributeOptions.Attribute = &timestampAttributeData;
			EOS_LobbyModification_AddAttribute(modificationHandle, &attributeOptions);

			if(lobbyManager->_createLobbyName)
			{
				EOS_Lobby_AttributeData attributeData = {0};
				attributeData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
				attributeData.ValueType = EOS_EAttributeType::EOS_AT_STRING;
				attributeData.Key = "lobbyName";
				attributeData.Value.AsUtf8 = lobbyManager->_createLobbyName->GetUTF8String();
				attributeOptions.Attribute = &attributeData;
				EOS_LobbyModification_AddAttribute(modificationHandle, &attributeOptions);

				//Can be used to filter with to then search by name locally in the returned results
				String *lobbyNameStub = lobbyManager->_createLobbyName->GetSubstring(Range(0, std::min(static_cast<size_t>(3), lobbyManager->_createLobbyName->GetLength())));
				lobbyNameStub->MakeLowercase();

				EOS_Lobby_AttributeData stubAttributeData = {0};
				stubAttributeData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
				stubAttributeData.ValueType = EOS_EAttributeType::EOS_AT_STRING;
				stubAttributeData.Key = "lobbyNameSearchStub";
				stubAttributeData.Value.AsUtf8 = lobbyNameStub->GetUTF8String();
				attributeOptions.Attribute = &stubAttributeData;
				EOS_LobbyModification_AddAttribute(modificationHandle, &attributeOptions);

				SafeRelease(lobbyManager->_createLobbyName);
			}

			if(lobbyManager->_createLobbyLevel)
			{
				EOS_Lobby_AttributeData attributeData = {0};
				attributeData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
				attributeData.ValueType = EOS_EAttributeType::EOS_AT_STRING;
				attributeData.Key = "lobbyLevel";
				attributeData.Value.AsUtf8 = lobbyManager->_createLobbyLevel->GetUTF8String();
				attributeOptions.Attribute = &attributeData;
				EOS_LobbyModification_AddAttribute(modificationHandle, &attributeOptions);
				SafeRelease(lobbyManager->_createLobbyLevel);
			}

			if(lobbyManager->_createLobbyVersion)
			{
				EOS_Lobby_AttributeData attributeData = {0};
				attributeData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
				attributeData.ValueType = EOS_EAttributeType::EOS_AT_STRING;
				attributeData.Key = "lobbyVersion";
				attributeData.Value.AsUtf8 = lobbyManager->_createLobbyVersion->GetUTF8String();
				attributeOptions.Attribute = &attributeData;
				EOS_LobbyModification_AddAttribute(modificationHandle, &attributeOptions);
				SafeRelease(lobbyManager->_createLobbyVersion);
			}

			EOS_Lobby_UpdateLobbyOptions updateLobbyOptions = {0};
			updateLobbyOptions.ApiVersion = EOS_LOBBY_UPDATELOBBY_API_LATEST;
			updateLobbyOptions.LobbyModificationHandle = modificationHandle;

			EOS_Lobby_UpdateLobby(lobbyManager->_lobbyInterfaceHandle, &updateLobbyOptions, lobbyManager, LobbyOnUpdateCallback);

			if(lobbyManager->_didJoinLobbyCallback)
			{
				lobbyManager->_didJoinLobbyCallback(EOSResult::Success);
			}
		}
		else
		{
			RNDebug("Failed creating lobby: " << EOS_EResult_ToString(Data->ResultCode));
			if(lobbyManager->_didJoinLobbyCallback)
			{
				if(Data->ResultCode == EOS_EResult::EOS_NoConnection || Data->ResultCode == EOS_EResult::EOS_OperationWillRetry || Data->ResultCode == EOS_EResult::EOS_TimedOut)
				{
					lobbyManager->_didJoinLobbyCallback(EOSResult::NoConnection);
				}
				else if(Data->ResultCode == EOS_EResult::EOS_InvalidAuth)
				{
					lobbyManager->_didJoinLobbyCallback(EOSResult::NotLoggedIn);
				}
				else
				{
					lobbyManager->_didJoinLobbyCallback(EOSResult::Other);
				}
			}
		}

		lobbyManager->_isJoiningLobby = false;
	}

	void EOSLobbyManager::LobbyOnSearchCallback(const EOS_LobbySearch_FindCallbackInfo *Data)
	{
		EOSLobbySearch *searchData = static_cast<EOSLobbySearch *>(Data->ClientData);

		if(Data->ResultCode == EOS_EResult::EOS_Success)
		{
			RNDebug("Lobby search successful");

			EOS_LobbySearch_GetSearchResultCountOptions searchResultCountOptions = {0};
			searchResultCountOptions.ApiVersion = EOS_LOBBYSEARCH_GETSEARCHRESULTCOUNT_API_LATEST;

			uint32 resultCount = EOS_LobbySearch_GetSearchResultCount(searchData->handle, &searchResultCountOptions);
			RNDebug("Found " << resultCount << " lobbies");

			Array *lobbyInfoArray = new Array();
			for(int i = 0; i < resultCount; i++)
			{
				EOS_LobbySearch_CopySearchResultByIndexOptions copyOptions = {0};
				copyOptions.ApiVersion = EOS_LOBBYSEARCH_COPYSEARCHRESULTBYINDEX_API_LATEST;
				copyOptions.LobbyIndex = i;

				EOS_HLobbyDetails lobbyDetailsHandle = nullptr;
				if(EOS_LobbySearch_CopySearchResultByIndex(searchData->handle, &copyOptions, &lobbyDetailsHandle) != EOS_EResult::EOS_Success)
				{
					RNDebug("Failed fetching search result for lobby at index " << i);
					continue;
				}

				EOSLobbyInfo *lobbyInfo = new EOSLobbyInfo();
				lobbyInfoArray->AddObject(lobbyInfo->Autorelease());

				lobbyInfo->lobbyHandle = lobbyDetailsHandle;

				EOS_LobbyDetails_CopyAttributeByKeyOptions copyAttributesOptions = {0};
				copyAttributesOptions.ApiVersion = EOS_LOBBYDETAILS_COPYATTRIBUTEBYKEY_API_LATEST;
				copyAttributesOptions.AttrKey = "lobbyName";
				EOS_Lobby_Attribute *lobbyNameAttribute = nullptr;
				EOS_LobbyDetails_CopyAttributeByKey(lobbyDetailsHandle, &copyAttributesOptions, &lobbyNameAttribute);
				if(lobbyNameAttribute)
				{
					lobbyInfo->lobbyName = new String(lobbyNameAttribute->Data->Value.AsUtf8);
					EOS_Lobby_Attribute_Release(lobbyNameAttribute);
				}

				copyAttributesOptions.AttrKey = "lobbyLevel";
				EOS_Lobby_Attribute *lobbyLevelAttribute = nullptr;
				EOS_LobbyDetails_CopyAttributeByKey(lobbyDetailsHandle, &copyAttributesOptions, &lobbyLevelAttribute);
				if(lobbyLevelAttribute)
				{
					lobbyInfo->lobbyLevel = new String(lobbyLevelAttribute->Data->Value.AsUtf8);
					EOS_Lobby_Attribute_Release(lobbyLevelAttribute);
				}

				copyAttributesOptions.AttrKey = "lobbyVersion";
				EOS_Lobby_Attribute *lobbyVersionAttribute = nullptr;
				EOS_LobbyDetails_CopyAttributeByKey(lobbyDetailsHandle, &copyAttributesOptions, &lobbyVersionAttribute);
				if(lobbyVersionAttribute)
				{
					lobbyInfo->lobbyVersion = new String(lobbyVersionAttribute->Data->Value.AsUtf8);
					EOS_Lobby_Attribute_Release(lobbyVersionAttribute);
				}

				copyAttributesOptions.AttrKey = "timestamp";
				EOS_Lobby_Attribute *timestampAttribute = nullptr;
				EOS_LobbyDetails_CopyAttributeByKey(lobbyDetailsHandle, &copyAttributesOptions, &timestampAttribute);
				if(timestampAttribute)
				{
					lobbyInfo->createTimestamp = timestampAttribute->Data->Value.AsInt64;
					EOS_Lobby_Attribute_Release(timestampAttribute);
				}

				copyAttributesOptions.AttrKey = "hasPassword";
				EOS_Lobby_Attribute *hasPasswordAttribute = nullptr;
				EOS_LobbyDetails_CopyAttributeByKey(lobbyDetailsHandle, &copyAttributesOptions, &hasPasswordAttribute);
				if(hasPasswordAttribute)
				{
					lobbyInfo->hasPassword = hasPasswordAttribute->Data->Value.AsBool;
					EOS_Lobby_Attribute_Release(hasPasswordAttribute);
				}


				EOS_LobbyDetails_CopyInfoOptions copyInfoOptions = {0};
				copyInfoOptions.ApiVersion = EOS_LOBBYDETAILS_COPYINFO_API_LATEST;
				EOS_LobbyDetails_Info *lobbyDetailsInfo = nullptr;
				EOS_LobbyDetails_CopyInfo(lobbyDetailsHandle, &copyInfoOptions, &lobbyDetailsInfo);

				if(lobbyDetailsInfo)
				{
					lobbyInfo->maximumPlayerCount = lobbyDetailsInfo->MaxMembers;
					lobbyInfo->currentPlayerCount = lobbyInfo->maximumPlayerCount - lobbyDetailsInfo->AvailableSlots;
					lobbyInfo->ownerHandle = lobbyDetailsInfo->LobbyOwnerUserId;
					EOS_LobbyDetails_Info_Release(lobbyDetailsInfo);
				}
			}

			if(searchData->callback)
			{
				searchData->callback(EOSResult::Success, lobbyInfoArray);
				searchData->callback = nullptr;
			}

			lobbyInfoArray->Release();
		}
		else
		{
			RNDebug("Failed searching lobbies");
			if(searchData->callback)
			{
				if(Data->ResultCode == EOS_EResult::EOS_NoConnection || Data->ResultCode == EOS_EResult::EOS_OperationWillRetry || Data->ResultCode == EOS_EResult::EOS_TimedOut)
				{
					searchData->callback(EOSResult::NoConnection, nullptr);
				}
				else if(Data->ResultCode == EOS_EResult::EOS_InvalidAuth)
				{
					searchData->callback(EOSResult::NotLoggedIn, nullptr);
				}
				else
				{
					searchData->callback(EOSResult::Other, nullptr);
				}

				//On android this callback will be triggered 3 more times if there is no connection,
				//Unset the callback here to not have it triggered again until another search...
				searchData->callback = nullptr;
			}
		}

		EOS_LobbySearch_Release(searchData->handle);
		searchData->handle = nullptr;
	}

	EOS_ProductUserId EOSLobbyManager::GetLobbyOwnerID()
	{
		RN_ASSERT(_isConnectedToLobby, "Cannot query owner of lobby: not connected to any lobby.");

		EOS_LobbyDetails_GetLobbyOwnerOptions getLobbyOwnerOptions = {0};
		getLobbyOwnerOptions.ApiVersion = EOS_LOBBYDETAILS_GETLOBBYOWNER_API_LATEST;
		return EOS_LobbyDetails_GetLobbyOwner(_lobbyDetails, &getLobbyOwnerOptions);
	}

	void EOSLobbyManager::LobbyOnUpdateCallback(const EOS_Lobby_UpdateLobbyCallbackInfo *Data)
	{
		if(Data->ResultCode == EOS_EResult::EOS_Success)
		{
			RNDebug("Lobby parameters updated successfully");
		}
		else
		{
			RNDebug("Lobby parameter update failed");
		}
	}

	void EOSLobbyManager::LobbyOnJoinCallback(const EOS_Lobby_JoinLobbyCallbackInfo *Data)
	{
		EOSLobbyManager *lobbyManager = static_cast<EOSLobbyManager *>(Data->ClientData);

		if(Data->ResultCode == EOS_EResult::EOS_Success)
		{
			RNDebug("Joined lobby successfully");

			lobbyManager->_isConnectedToLobby = true;
			lobbyManager->_connectedLobbyID = new String(Data->LobbyId);

			EOS_Lobby_CopyLobbyDetailsHandleOptions copyOptions;
			copyOptions.ApiVersion = EOS_LOBBY_COPYLOBBYDETAILSHANDLE_API_LATEST;
			copyOptions.LocalUserId = EOSWorld::GetInstance()->GetUserID();
			copyOptions.LobbyId = Data->LobbyId;
			EOS_EResult copyDetailsResult = EOS_Lobby_CopyLobbyDetailsHandle(lobbyManager->_lobbyInterfaceHandle, &copyOptions, &lobbyManager->_lobbyDetails);
			if(copyDetailsResult == EOS_EResult::EOS_Success)
			{
				lobbyManager->RetrievePeers();
			}

			if(lobbyManager->_didJoinLobbyCallback)
			{
				lobbyManager->_didJoinLobbyCallback(EOSResult::Success);
			}

			if(lobbyManager->_isVoiceEnabled && (lobbyManager->_audioReceivedCallback || lobbyManager->_audioBeforeSendCallback))
			{
				EOS_Lobby_GetRTCRoomNameOptions roomNameOptions = {};
				roomNameOptions.ApiVersion = EOS_LOBBY_GETRTCROOMNAME_API_LATEST;
				roomNameOptions.LobbyId = lobbyManager->_connectedLobbyID->GetUTF8String();
				roomNameOptions.LocalUserId = EOSWorld::GetInstance()->GetUserID();
				char roomNameBuffer[512];
				RN::uint32 roomNameLength = 512;
				if(EOS_Lobby_GetRTCRoomName(lobbyManager->_lobbyInterfaceHandle, &roomNameOptions, roomNameBuffer, &roomNameLength) == EOS_EResult::EOS_Success)
				{
					if(lobbyManager->_audioReceivedCallback)
					{
						EOS_RTCAudio_AddNotifyAudioBeforeRenderOptions options = {};
						options.ApiVersion = EOS_RTCAUDIO_ADDNOTIFYAUDIOBEFORERENDER_API_LATEST;
						options.LocalUserId = EOSWorld::GetInstance()->GetUserID();
						options.bUnmixedAudio = lobbyManager->_isVoiceUnmixed;
						options.RoomName = roomNameBuffer;

						lobbyManager->_currentAudioBeforeRenderNotificationID = EOS_RTCAudio_AddNotifyAudioBeforeRender(lobbyManager->_rtcAudioInterfaceHandle, &options, lobbyManager, LobbyAudioOnBeforeRenderCallback);
					}

					if(lobbyManager->_audioBeforeSendCallback)
					{
						EOS_RTCAudio_AddNotifyAudioBeforeSendOptions options = {};
						options.ApiVersion = EOS_RTCAUDIO_ADDNOTIFYAUDIOBEFORESEND_API_LATEST;
						options.LocalUserId = EOSWorld::GetInstance()->GetUserID();
						options.RoomName = roomNameBuffer;

						lobbyManager->_currentAudioBeforeSendNotificationID = EOS_RTCAudio_AddNotifyAudioBeforeSend(lobbyManager->_rtcAudioInterfaceHandle, &options, lobbyManager, LobbyAudioOnBeforeSendCallback);
					}
				}
			}

			if(lobbyManager->_isVoiceEnabled)
			{
				EOS_RTCAudio_SetInputDeviceSettingsOptions audioInputSettings = {};
				audioInputSettings.ApiVersion = EOS_RTCAUDIO_SETINPUTDEVICESETTINGS_API_LATEST;
				audioInputSettings.LocalUserId = EOSWorld::GetInstance()->GetUserID();
				audioInputSettings.bPlatformAEC = EOS_TRUE;
				EOS_RTCAudio_SetInputDeviceSettings(lobbyManager->_rtcAudioInterfaceHandle, &audioInputSettings, nullptr, nullptr);
			}
		}
		else
		{
			RNDebug("Failed joining lobby");
			if(lobbyManager->_didJoinLobbyCallback)
			{
				if(Data->ResultCode == EOS_EResult::EOS_NoConnection || Data->ResultCode == EOS_EResult::EOS_TimedOut || Data->ResultCode == EOS_EResult::EOS_OperationWillRetry)
				{
					lobbyManager->_didJoinLobbyCallback(EOSResult::NoConnection);
				}
				else if(Data->ResultCode == EOS_EResult::EOS_InvalidAuth)
				{
					lobbyManager->_didJoinLobbyCallback(EOSResult::NotLoggedIn);
				}
				else
				{
					lobbyManager->_didJoinLobbyCallback(EOSResult::Other);
				}
			}
		}

		lobbyManager->_isJoiningLobby = false;
	}

	void EOSLobbyManager::LobbyOnLeaveCallback(const EOS_Lobby_LeaveLobbyCallbackInfo *Data)
	{
		EOSLobbyManager *lobbyManager = static_cast<EOSLobbyManager *>(Data->ClientData);

		if(Data->ResultCode == EOS_EResult::EOS_Success)
		{
			RNDebug("Left lobby successfully");
		}
		else
		{
			RNDebug("Failed leaving lobby");
		}

		lobbyManager->_isJoiningLobby = false;
		lobbyManager->_isConnectedToLobby = false;
		lobbyManager->_isConnectedLobbyOwner = false;
		SafeRelease(lobbyManager->_connectedLobbyID);

		if(lobbyManager->_currentAudioBeforeRenderNotificationID != 0)
		{
			EOS_RTCAudio_RemoveNotifyAudioBeforeRender(lobbyManager->_rtcAudioInterfaceHandle, lobbyManager->_currentAudioBeforeRenderNotificationID);
			lobbyManager->_currentAudioBeforeRenderNotificationID = 0;
		}

		if(lobbyManager->_currentAudioBeforeSendNotificationID != 0)
		{
			EOS_RTCAudio_RemoveNotifyAudioBeforeSend(lobbyManager->_rtcAudioInterfaceHandle, lobbyManager->_currentAudioBeforeSendNotificationID);
			lobbyManager->_currentAudioBeforeSendNotificationID = 0;
		}
	}

	void EOSLobbyManager::LobbyOnMemberStatusReceived(const EOS_Lobby_LobbyMemberStatusReceivedCallbackInfo *Data)
	{
		EOSLobbyManager *lobbyManager = static_cast<EOSLobbyManager *>(Data->ClientData);
		RNDebug("Lobby member status received for " << Data->TargetUserId << " in lobby " << Data->LobbyId);

		EOS_Lobby_CopyLobbyDetailsHandleOptions copyOptions;
		copyOptions.ApiVersion = EOS_LOBBY_COPYLOBBYDETAILSHANDLE_API_LATEST;
		EOS_ProductUserId localProductUserID = EOSWorld::GetInstance()->GetUserID();
		copyOptions.LocalUserId = localProductUserID;
		copyOptions.LobbyId = Data->LobbyId;
		EOS_Lobby_CopyLobbyDetailsHandle(lobbyManager->_lobbyInterfaceHandle, &copyOptions, &lobbyManager->_lobbyDetails);

		switch(Data->CurrentStatus)
		{
			case EOS_ELobbyMemberStatus::EOS_LMS_JOINED:
				RNDebug("Member joined: " << Data->TargetUserId);
				lobbyManager->AddRemotePeer(Data->TargetUserId);
				break;
			case EOS_ELobbyMemberStatus::EOS_LMS_LEFT:
				RNDebug("Member left: " << Data->TargetUserId);
				lobbyManager->RemoveRemotePeer(Data->TargetUserId);
				break;
			case EOS_ELobbyMemberStatus::EOS_LMS_CLOSED:
				RNDebug("Lobby closed: " << Data->TargetUserId);
				EOSWorld::GetInstance()->Disconnect();
				break;
			case EOS_ELobbyMemberStatus::EOS_LMS_KICKED:
			case EOS_ELobbyMemberStatus::EOS_LMS_DISCONNECTED:
				RNDebug("Member disconnected from lobby: " << Data->TargetUserId);
				lobbyManager->RemoveRemotePeer(Data->TargetUserId);
				if(Data->TargetUserId == localProductUserID)
				{
					EOSWorld::GetInstance()->Disconnect();
				}
				else
				{
					EOSWorld::GetInstance()->Disconnect(Data->TargetUserId);
				}
				break;
			case EOS_ELobbyMemberStatus::EOS_LMS_PROMOTED:
			{
				RNDebug("Host migrating to: " << Data->TargetUserId);
				lobbyManager->_isConnectedLobbyOwner = EOSWorld::GetInstance()->GetUserID() == Data->TargetUserId;
				EOSWorld::GetInstance()->MigrateHost(Data->TargetUserId);
			}

			break;
			default:
				break;
		}
	}

	void EOSLobbyManager::LobbyOnDestroyCallback(const EOS_Lobby_DestroyLobbyCallbackInfo *Data)
	{
		EOSLobbyManager *lobbyManager = static_cast<EOSLobbyManager *>(Data->ClientData);

		if(Data->ResultCode == EOS_EResult::EOS_Success)
		{
			RNDebug("Destroyed lobby successfully");
		}
		else
		{
			RNDebug("Failed destroying lobby");
		}

		lobbyManager->_isJoiningLobby = false;
		lobbyManager->_isConnectedToLobby = false;
		lobbyManager->_isConnectedLobbyOwner = false;
		SafeRelease(lobbyManager->_connectedLobbyID);

		if(lobbyManager->_currentAudioBeforeRenderNotificationID != 0)
		{
			EOS_RTCAudio_RemoveNotifyAudioBeforeRender(lobbyManager->_rtcAudioInterfaceHandle, lobbyManager->_currentAudioBeforeRenderNotificationID);
			lobbyManager->_currentAudioBeforeRenderNotificationID = 0;
		}

		if(lobbyManager->_currentAudioBeforeSendNotificationID != 0)
		{
			EOS_RTCAudio_RemoveNotifyAudioBeforeSend(lobbyManager->_rtcAudioInterfaceHandle, lobbyManager->_currentAudioBeforeSendNotificationID);
			lobbyManager->_currentAudioBeforeSendNotificationID = 0;
		}
	}

	void EOSLobbyManager::LobbyOnKickMemberCallback(const EOS_Lobby_KickMemberCallbackInfo *Data)
	{
		if(Data->ResultCode == EOS_EResult::EOS_Success)
		{
			RNDebug("Kicked user successfully");
		}
		else
		{
			RNDebug("Failed kicking user");
		}
	}

	void EOSLobbyManager::LobbyAudioOnBeforeSendCallback(const EOS_RTCAudio_AudioBeforeSendCallbackInfo *Data)
	{
		EOSLobbyManager *lobbyManager = static_cast<EOSLobbyManager *>(Data->ClientData);
		if(lobbyManager->_audioBeforeSendCallback)
		{
			lobbyManager->_audioBeforeSendCallback(Data->Buffer->SampleRate, Data->Buffer->Channels, Data->Buffer->FramesCount, Data->Buffer->Frames);
		}
	}

	void EOSLobbyManager::LobbyAudioOnBeforeRenderCallback(const EOS_RTCAudio_AudioBeforeRenderCallbackInfo *Data)
	{
		EOSLobbyManager *lobbyManager = static_cast<EOSLobbyManager *>(Data->ClientData);

		if(lobbyManager->_audioReceivedCallback)
		{
			AutoreleasePool pool;

			RN::String *eosUserID = nullptr;

			if(Data->ParticipantId)
			{
				eosUserID = EOSWorld::GetInstance()->GetUserIDString(Data->ParticipantId);
			}
			lobbyManager->_audioReceivedCallback(eosUserID, Data->Buffer->SampleRate, Data->Buffer->Channels, Data->Buffer->FramesCount, Data->Buffer->Frames);
		}
	}

	void EOSLobbyManager::LobbyAudioOnUpdateSendingCallback(const EOS_RTCAudio_UpdateSendingCallbackInfo *Data)
	{
	}
} // namespace RN
