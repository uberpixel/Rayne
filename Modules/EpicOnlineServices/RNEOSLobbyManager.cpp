//
//  RNEOSLobbyManager.cpp
//  Rayne-EOS
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNEOSLobbyManager.h"
#include "RNEOSWorld.h"

#include "eos_platform_prereqs.h"
#include "eos_sdk.h"
#include "eos_common.h"

#include "eos_lobby.h"
#include "eos_lobby_types.h"
#include "eos_rtc.h"
#include "eos_rtc_audio.h"
#include "eos_rtc_audio_types.h"

namespace RN
{
	RNDefineMeta(EOSLobbyInfo, Object)
	RNDefineMeta(EOSLobbyManager, Object)

	EOSLobbyInfo::EOSLobbyInfo(): lobbyName(nullptr), lobbyLevel(nullptr), lobbyVersion(nullptr), maximumPlayerCount(0),  currentPlayerCount(0), lobbyHandle(nullptr), ownerHandle(nullptr), createTimestamp(0)
	{
		
	}

	EOSLobbyInfo::~EOSLobbyInfo()
	{
		SafeRelease(lobbyName);
		SafeRelease(lobbyLevel);
		SafeRelease(lobbyVersion);
		EOS_LobbyDetails_Release(lobbyHandle);
	}

	const String *EOSLobbyInfo::GetDescription() const
	{
		return RNSTR("<" << GetClass()->GetFullname() << ":" << (void *)this << ">\n{\n	lobbyName: " << lobbyName << ",\n	lobbyLevel: " << lobbyLevel << ",\n	lobbyVersion: " << lobbyVersion << ",\n	maximumPlayerCount: " << maximumPlayerCount << ",\n	currentPlayerCount: " << currentPlayerCount << "\n}");
	}

	EOSLobbyManager::EOSLobbyManager(EOSWorld *world) : _createLobbyName(nullptr), _createLobbyVersion(nullptr), _isSearchingLobby(false), _isJoiningLobby(false), _didJoinLobbyCallback(nullptr), _lobbySearchCallback(nullptr), _isConnectedToLobby(false), _connectedLobbyID(nullptr), _isConnectedLobbyOwner(false), _isVoiceEnabled(false), _isVoiceUnmixed(true), _audioReceivedCallback(nullptr)
	{
		_lobbyInterfaceHandle = EOS_Platform_GetLobbyInterface(world->GetPlatformHandle());
		
		_rtcInterfaceHandle = EOS_Platform_GetRTCInterface(world->GetPlatformHandle());
		_rtcAudioInterfaceHandle = EOS_RTC_GetAudioInterface(_rtcInterfaceHandle);
	}
		
	EOSLobbyManager::~EOSLobbyManager()
	{
		SafeRelease(_connectedLobbyID);
	}

	void EOSLobbyManager::SetGlobalAudioOptions(bool voiceEnabled, bool unmixed, std::function<void(RN::String *eosUserID, RN::uint32 sampleRate, RN::uint32 channels, RN::uint32 framesCount, RN::int16 *frames)> audioReceivedCallback)
	{
		_audioReceivedCallback = audioReceivedCallback;
		_isVoiceEnabled = voiceEnabled;
		_isVoiceUnmixed = unmixed;
	}

	void EOSLobbyManager::SetLocalPlayerMute(bool mute)
	{
		_isLocalPlayerMuted = mute;
		
		if(_isConnectedToLobby)
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
				sendingOptions.AudioStatus = mute? EOS_ERTCAudioStatus::EOS_RTCAS_Disabled : EOS_ERTCAudioStatus::EOS_RTCAS_Enabled;
				EOS_RTCAudio_UpdateSending(_rtcAudioInterfaceHandle, &sendingOptions, this, LobbyAudioOnUpdateSendingCallback);
			}
		}
	}

	void EOSLobbyManager::CreateLobby(int64 createLobbyTimestamp, String *lobbyName, String *lobbyLevel, uint8 maxUsers, std::function<void(bool)> callback, String *lobbyVersion, bool hasPassword, const String *lobbyIDOverride)
	{
		if(EOSWorld::GetInstance()->GetLoginState() != EOSWorld::LoginStateIsLoggedIn)
		{
			if(callback) callback(false);
			return;
		}
		if(_isJoiningLobby || _isConnectedToLobby)
		{
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
		options.PermissionLevel = maxUsers==1?EOS_ELobbyPermissionLevel::EOS_LPL_INVITEONLY : EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED;
		options.bPresenceEnabled = false;
		options.bDisableHostMigration = true; //Host migration is currently not supported with my p2p setup, so disabling it for lobbies should help with event lobbies not disappearing
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

	void EOSLobbyManager::SearchLobby(bool includePrivate, bool includePublic, uint32 maxResults, std::function<void(bool, RN::Array *)> callback, const RN::String *lobbyID)
	{
		if(EOSWorld::GetInstance()->GetLoginState() != EOSWorld::LoginStateIsLoggedIn)
		{
			if(callback) callback(false, nullptr);
			return;
		}
		if(_isSearchingLobby) return;
		
		EOS_Lobby_CreateLobbySearchOptions searchOptions = {0};
		searchOptions.ApiVersion = EOS_LOBBY_CREATELOBBYSEARCH_API_LATEST;
		searchOptions.MaxResults = std::min(maxResults, static_cast<uint32>(EOS_LOBBY_MAX_SEARCH_RESULTS));
		
		if(EOS_Lobby_CreateLobbySearch(_lobbyInterfaceHandle, &searchOptions, &_lobbySearchHandle) != EOS_EResult::EOS_Success)
		{
			RNDebug("Failed creating EOS Lobby search handle");
			return;
		}
		
		_isSearchingLobby = true;
		_lobbySearchCallback = callback;
		
		if(lobbyID)
		{
			EOS_LobbySearch_SetLobbyIdOptions lobbyIDOptions = {0};
			lobbyIDOptions.ApiVersion = EOS_LOBBYSEARCH_SETLOBBYID_API_LATEST;
			lobbyIDOptions.LobbyId = lobbyID->GetUTF8String();
			EOS_LobbySearch_SetLobbyId(_lobbySearchHandle, &lobbyIDOptions);
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
			
			EOS_LobbySearch_SetParameter(_lobbySearchHandle, &timestampSearchParameterOptions);
			
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
				
				EOS_LobbySearch_SetParameter(_lobbySearchHandle, &searchParameterOptions);
			}
		}
		
		EOS_LobbySearch_FindOptions findOptions = {0};
		findOptions.ApiVersion = EOS_LOBBYSEARCH_FIND_API_LATEST;
		findOptions.LocalUserId = EOSWorld::GetInstance()->GetUserID();
		
		RNDebug("Start searching lobbies");
		EOS_LobbySearch_Find(_lobbySearchHandle, &findOptions, this, LobbyOnSearchCallback);
	}

	void EOSLobbyManager::JoinLobby(EOSLobbyInfo *lobbyInfo, std::function<void(bool)> callback)
	{
		if(EOSWorld::GetInstance()->GetLoginState() != EOSWorld::LoginStateIsLoggedIn || _isJoiningLobby || _isConnectedToLobby) return;
		
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

	void EOSLobbyManager::LeaveCurrentLobby()
	{
		if(!_isConnectedToLobby) return;
		
		if(_isConnectedLobbyOwner)
		{
			EOS_Lobby_DestroyLobbyOptions destroyOptions = {0};
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
		
		attributes->Enumerate([&](Object *object, const Object *key, bool &stop){
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
		_lobbySearchCallback = nullptr;
	}

	void EOSLobbyManager::LobbyOnCreateCallback(const EOS_Lobby_CreateLobbyCallbackInfo* Data)
	{
		EOSLobbyManager *lobbyManager = static_cast<EOSLobbyManager*>(Data->ClientData);
		
		if(Data->ResultCode == EOS_EResult::EOS_Success)
		{
			RNDebug("Lobby successfully created with ID: " << Data->LobbyId);
			
			lobbyManager->_isConnectedToLobby = true;
			lobbyManager->_connectedLobbyID = new String(Data->LobbyId);
			lobbyManager->_isConnectedLobbyOwner = true;
			
			if(lobbyManager->_isVoiceEnabled && lobbyManager->_audioReceivedCallback)
			{
				EOS_Lobby_GetRTCRoomNameOptions roomNameOptions = {};
				roomNameOptions.ApiVersion = EOS_LOBBY_GETRTCROOMNAME_API_LATEST;
				roomNameOptions.LobbyId = lobbyManager->_connectedLobbyID->GetUTF8String();
				roomNameOptions.LocalUserId = EOSWorld::GetInstance()->GetUserID();
				char roomNameBuffer[512];
				RN::uint32 roomNameLength = 512;
				if(EOS_Lobby_GetRTCRoomName(lobbyManager->_lobbyInterfaceHandle, &roomNameOptions, roomNameBuffer, &roomNameLength) == EOS_EResult::EOS_Success)
				{
					EOS_RTCAudio_AddNotifyAudioBeforeRenderOptions options = {};
					options.ApiVersion = EOS_RTCAUDIO_ADDNOTIFYAUDIOBEFORERENDER_API_LATEST;
					options.LocalUserId = EOSWorld::GetInstance()->GetUserID();
					options.bUnmixedAudio = lobbyManager->_isVoiceUnmixed;
					options.RoomName = roomNameBuffer;
					
					EOS_RTCAudio_AddNotifyAudioBeforeRender(lobbyManager->_rtcAudioInterfaceHandle, &options, lobbyManager, LobbyAudioOnBeforeRenderCallback);
				}
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
				lobbyManager->_didJoinLobbyCallback(true);
			}
		}
		else
		{
			RNDebug("Failed creating lobby: " << EOS_EResult_ToString(Data->ResultCode));
			if(lobbyManager->_didJoinLobbyCallback)
			{
				lobbyManager->_didJoinLobbyCallback(false);
			}
		}
		
		lobbyManager->_isJoiningLobby = false;
	}

	void EOSLobbyManager::LobbyOnSearchCallback(const EOS_LobbySearch_FindCallbackInfo *Data)
	{
		EOSLobbyManager *lobbyManager = static_cast<EOSLobbyManager*>(Data->ClientData);
		
		if(Data->ResultCode == EOS_EResult::EOS_Success)
		{
			RNDebug("Lobby search successful");
			
			EOS_LobbySearch_GetSearchResultCountOptions searchResultCountOptions = {0};
			searchResultCountOptions.ApiVersion = EOS_LOBBYSEARCH_GETSEARCHRESULTCOUNT_API_LATEST;
			
			uint32 resultCount = EOS_LobbySearch_GetSearchResultCount(lobbyManager->_lobbySearchHandle, &searchResultCountOptions);
			RNDebug("Found " << resultCount << " lobbies");
			
			Array *lobbyInfoArray = new Array();
			for(int i = 0; i < resultCount; i++)
			{
				EOS_LobbySearch_CopySearchResultByIndexOptions copyOptions = {0};
				copyOptions.ApiVersion = EOS_LOBBYSEARCH_COPYSEARCHRESULTBYINDEX_API_LATEST;
				copyOptions.LobbyIndex = i;
				
				EOS_HLobbyDetails lobbyDetailsHandle = nullptr;
				if(EOS_LobbySearch_CopySearchResultByIndex(lobbyManager->_lobbySearchHandle, &copyOptions, &lobbyDetailsHandle) != EOS_EResult::EOS_Success)
				{
					RNDebug("Failed fetching search result for lobby at index " << i);
					continue;
				}
				
				String *lobbyID = nullptr;
				uint8 currentPlayerCount = 0;
				uint8 maxPlayerCount = 0;
				
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
			
			if(lobbyManager->_lobbySearchCallback)
			{
				lobbyManager->_lobbySearchCallback(true, lobbyInfoArray);
			}
			
			lobbyInfoArray->Release();
		}
		else
		{
			RNDebug("Failed searching lobbies");
			if(lobbyManager->_lobbySearchCallback)
			{
				lobbyManager->_lobbySearchCallback(false, nullptr);
			}
			//On android this callback will be triggered 3 more times if there is no connection,
			//Unset the callback here to not have it triggered again until another search...
			lobbyManager->_lobbySearchCallback = nullptr;
		}
		lobbyManager->_isSearchingLobby = false;
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
		EOSLobbyManager *lobbyManager = static_cast<EOSLobbyManager*>(Data->ClientData);
		
		if(Data->ResultCode == EOS_EResult::EOS_Success)
		{
			RNDebug("Joined lobby successfully");
			
			lobbyManager->_isConnectedToLobby = true;
			lobbyManager->_connectedLobbyID = new String(Data->LobbyId);
			if(lobbyManager->_didJoinLobbyCallback)
			{
				lobbyManager->_didJoinLobbyCallback(true);
			}
			
			if(lobbyManager->_isVoiceEnabled && lobbyManager->_audioReceivedCallback)
			{
				EOS_Lobby_GetRTCRoomNameOptions roomNameOptions = {};
				roomNameOptions.ApiVersion = EOS_LOBBY_GETRTCROOMNAME_API_LATEST;
				roomNameOptions.LobbyId = lobbyManager->_connectedLobbyID->GetUTF8String();
				roomNameOptions.LocalUserId = EOSWorld::GetInstance()->GetUserID();
				char roomNameBuffer[512];
				RN::uint32 roomNameLength = 512;
				if(EOS_Lobby_GetRTCRoomName(lobbyManager->_lobbyInterfaceHandle, &roomNameOptions, roomNameBuffer, &roomNameLength) == EOS_EResult::EOS_Success)
				{
					EOS_RTCAudio_AddNotifyAudioBeforeRenderOptions options = {};
					options.ApiVersion = EOS_RTCAUDIO_ADDNOTIFYAUDIOBEFORERENDER_API_LATEST;
					options.LocalUserId = EOSWorld::GetInstance()->GetUserID();
					options.bUnmixedAudio = lobbyManager->_isVoiceUnmixed;
					options.RoomName = roomNameBuffer;
					
					EOS_RTCAudio_AddNotifyAudioBeforeRender(lobbyManager->_rtcAudioInterfaceHandle, &options, lobbyManager, LobbyAudioOnBeforeRenderCallback);
				}
			}
		}
		else
		{
			RNDebug("Failed joining lobby");
			if(lobbyManager->_didJoinLobbyCallback)
			{
				lobbyManager->_didJoinLobbyCallback(false);
			}
		}
		
		lobbyManager->_isJoiningLobby = false;
	}

	void EOSLobbyManager::LobbyOnLeaveCallback(const EOS_Lobby_LeaveLobbyCallbackInfo *Data)
	{
		EOSLobbyManager *lobbyManager = static_cast<EOSLobbyManager*>(Data->ClientData);
		
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
	}

	void EOSLobbyManager::LobbyOnDestroyCallback(const EOS_Lobby_DestroyLobbyCallbackInfo *Data)
	{
		EOSLobbyManager *lobbyManager = static_cast<EOSLobbyManager*>(Data->ClientData);
		
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

	void EOSLobbyManager::LobbyAudioOnBeforeRenderCallback(const EOS_RTCAudio_AudioBeforeRenderCallbackInfo *Data)
	{
		EOSLobbyManager *lobbyManager = static_cast<EOSLobbyManager*>(Data->ClientData);
		
		if(lobbyManager->_audioReceivedCallback)
		{
			RN::String *eosUserID = nullptr;
			
			if(Data->ParticipantId)
			{
				char outBuffer[EOS_PRODUCTUSERID_MAX_LENGTH + 1];
				int32_t outBufferLength = EOS_PRODUCTUSERID_MAX_LENGTH + 1;
				if(EOS_ProductUserId_ToString(Data->ParticipantId, outBuffer, &outBufferLength) == EOS_EResult::EOS_Success)
				{
					eosUserID = new RN::String(outBuffer);
				}
			}
			lobbyManager->_audioReceivedCallback(eosUserID, Data->Buffer->SampleRate, Data->Buffer->Channels, Data->Buffer->FramesCount, Data->Buffer->Frames);
			
			if(eosUserID) eosUserID->Release();
		}
	}

	void EOSLobbyManager::LobbyAudioOnUpdateSendingCallback(const EOS_RTCAudio_UpdateSendingCallbackInfo *Data)
	{
		
	}
}
