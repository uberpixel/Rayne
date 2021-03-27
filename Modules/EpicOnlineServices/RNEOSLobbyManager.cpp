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

namespace RN
{
	RNDefineMeta(EOSLobbyInfo, Object)
	RNDefineMeta(EOSLobbyManager, Object)

	EOSLobbyInfo::EOSLobbyInfo(): lobbyName(nullptr), maximumPlayerCount(0),  currentPlayerCount(0), lobbyHandle(nullptr), ownerHandle(nullptr)
	{
		
	}

	EOSLobbyInfo::~EOSLobbyInfo()
	{
		SafeRelease(lobbyName);
		EOS_LobbyDetails_Release(lobbyHandle);
	}

	EOSLobbyManager::EOSLobbyManager(EOSWorld *world) : _createLobbyName(nullptr), _isSearchingLobby(false), _isJoiningLobby(false), _didJoinLobbyCallback(nullptr), _lobbySearchCallback(nullptr), _isConnectedToLobby(false), _connectedLobbyID(nullptr), _isConnectedLobbyOwner(false)
	{
		_lobbyInterfaceHandle = EOS_Platform_GetLobbyInterface(world->GetPlatformHandle());
	}
		
	EOSLobbyManager::~EOSLobbyManager()
	{
		SafeRelease(_connectedLobbyID);
	}

	void EOSLobbyManager::CreateLobby(String *lobbyName, uint8 maxUsers, std::function<void()> callback)
	{
		if(!EOSWorld::GetInstance()->GetIsLoggedIn() || _isJoiningLobby || _isConnectedToLobby) return;
		
		_isJoiningLobby = true;
		_didJoinLobbyCallback = callback;
		_createLobbyName = SafeRetain(lobbyName);
		
		EOS_Lobby_CreateLobbyOptions options = {};
		options.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
		options.LocalUserId = EOSWorld::GetInstance()->GetUserID();
		options.MaxLobbyMembers = maxUsers;
		options.PermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED;
		options.bPresenceEnabled = false;
		EOS_Lobby_CreateLobby(_lobbyInterfaceHandle, &options, this, LobbyOnCreateCallback);
	}

	void EOSLobbyManager::SearchLobby(std::function<void (RN::Array *)> callback)
	{
		if(!EOSWorld::GetInstance()->GetIsLoggedIn() || _isSearchingLobby) return;
		
		EOS_Lobby_CreateLobbySearchOptions searchOptions = {};
		searchOptions.ApiVersion = EOS_LOBBY_CREATELOBBYSEARCH_API_LATEST;
		searchOptions.MaxResults = 100;
		
		if(EOS_Lobby_CreateLobbySearch(_lobbyInterfaceHandle, &searchOptions, &_lobbySearchHandle) != EOS_EResult::EOS_Success)
		{
			RNDebug("Failed creating EOS Lobby search handle");
			return;
		}
		
		_isSearchingLobby = true;
		_lobbySearchCallback = callback;
		
		EOS_Lobby_AttributeData attributeData = {0};
		attributeData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
		attributeData.ValueType = EOS_EAttributeType::EOS_AT_BOOLEAN;
		attributeData.Key = "isSearchable";
		attributeData.Value.AsBool = true;
		
		EOS_LobbySearch_SetParameterOptions searchParameterOptions = {0};
		searchParameterOptions.ApiVersion = EOS_LOBBYSEARCH_SETPARAMETER_API_LATEST;
		searchParameterOptions.ComparisonOp = EOS_EComparisonOp::EOS_CO_EQUAL;
		searchParameterOptions.Parameter = &attributeData;
		
		EOS_LobbySearch_SetParameter(_lobbySearchHandle, &searchParameterOptions);
		
		EOS_LobbySearch_FindOptions findOptions = {};
		findOptions.ApiVersion = EOS_LOBBYSEARCH_FIND_API_LATEST;
		findOptions.LocalUserId = EOSWorld::GetInstance()->GetUserID();
		
		EOS_LobbySearch_Find(_lobbySearchHandle, &findOptions, this, LobbyOnSearchCallback);
	}

	void EOSLobbyManager::JoinLobby(EOSLobbyInfo *lobbyInfo, std::function<void()> callback)
	{
		if(!EOSWorld::GetInstance()->GetIsLoggedIn() || _isJoiningLobby || _isConnectedToLobby) return;
		
		_isJoiningLobby = true;
		_didJoinLobbyCallback = callback;
		
		EOS_Lobby_JoinLobbyOptions joinOptions = {0};
		joinOptions.ApiVersion = EOS_LOBBY_JOINLOBBY_API_LATEST;
		joinOptions.LocalUserId = EOSWorld::GetInstance()->GetUserID();
		joinOptions.LobbyDetailsHandle = lobbyInfo->lobbyHandle;
		joinOptions.bPresenceEnabled = false;
		
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
			
			EOS_Lobby_AttributeData attributeData = {0};
			attributeData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
			attributeData.ValueType = EOS_EAttributeType::EOS_AT_BOOLEAN;
			attributeData.Key = "isSearchable";
			attributeData.Value.AsBool = true;
			
			EOS_LobbyModification_AddAttributeOptions attributeOptions = {0};
			attributeOptions.ApiVersion = EOS_LOBBYMODIFICATION_ADDATTRIBUTE_API_LATEST;
			attributeOptions.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC;
			attributeOptions.Attribute = &attributeData;
			
			EOS_LobbyModification_AddAttribute(modificationHandle, &attributeOptions);
			
			if(lobbyManager->_createLobbyName)
			{
				attributeData.ValueType = EOS_EAttributeType::EOS_AT_STRING;
				attributeData.Key = "lobbyName";
				attributeData.Value.AsUtf8 = lobbyManager->_createLobbyName->GetUTF8String();
				attributeOptions.Attribute = &attributeData;
				EOS_LobbyModification_AddAttribute(modificationHandle, &attributeOptions);
				SafeRelease(lobbyManager->_createLobbyName);
			}
			
			EOS_Lobby_UpdateLobbyOptions updateLobbyOptions = {0};
			updateLobbyOptions.ApiVersion = EOS_LOBBY_UPDATELOBBY_API_LATEST;
			updateLobbyOptions.LobbyModificationHandle = modificationHandle;
			
			EOS_Lobby_UpdateLobby(lobbyManager->_lobbyInterfaceHandle, &updateLobbyOptions, lobbyManager, LobbyOnUpdateCallback);
			
			if(lobbyManager->_didJoinLobbyCallback)
			{
				lobbyManager->_didJoinLobbyCallback();
			}
		}
		else
		{
			RNDebug("Failed creating lobby");
		}
		
		lobbyManager->_isJoiningLobby = false;
	}

	void EOSLobbyManager::LobbyOnSearchCallback(const EOS_LobbySearch_FindCallbackInfo *Data)
	{
		EOSLobbyManager *lobbyManager = static_cast<EOSLobbyManager*>(Data->ClientData);
		
		Array *lobbyInfoArray = new Array();
		
		if(Data->ResultCode == EOS_EResult::EOS_Success)
		{
			RNDebug("Lobby search successful");
			
			EOS_LobbySearch_GetSearchResultCountOptions searchResultCountOptions = {0};
			searchResultCountOptions.ApiVersion = EOS_LOBBYSEARCH_GETSEARCHRESULTCOUNT_API_LATEST;
			
			uint32 resultCount = EOS_LobbySearch_GetSearchResultCount(lobbyManager->_lobbySearchHandle, &searchResultCountOptions);
			RNDebug("Found " << resultCount << " lobbies");
			
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
		}
		else
		{
			RNDebug("Failed searching lobbies");
		}
		
		if(lobbyManager->_lobbySearchCallback)
		{
			lobbyManager->_lobbySearchCallback(lobbyInfoArray);
		}
		lobbyInfoArray->Release();
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
				lobbyManager->_didJoinLobbyCallback();
			}
		}
		else
		{
			RNDebug("Failed joining lobby");
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
}
