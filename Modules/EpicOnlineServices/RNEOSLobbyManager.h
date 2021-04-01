//
//  RNEOSLobbyManager.h
//  Rayne-EOS
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_EOSLOBBYMANAGER_H_
#define __RAYNE_EOSLOBBYMANAGER_H_

#include "RNEOS.h"

struct EOS_ProductUserIdDetails;
typedef struct EOS_ProductUserIdDetails* EOS_ProductUserId;

struct EOS_LobbyHandle;
typedef struct EOS_LobbyHandle* EOS_HLobby;

struct EOS_LobbySearchHandle;
typedef struct EOS_LobbySearchHandle* EOS_HLobbySearch;

struct EOS_LobbyDetailsHandle;
typedef struct EOS_LobbyDetailsHandle* EOS_HLobbyDetails;

typedef struct _tagEOS_Lobby_CreateLobbyCallbackInfo EOS_Lobby_CreateLobbyCallbackInfo;
typedef struct _tagEOS_LobbySearch_FindCallbackInfo EOS_LobbySearch_FindCallbackInfo;
typedef struct _tagEOS_Lobby_UpdateLobbyCallbackInfo EOS_Lobby_UpdateLobbyCallbackInfo;
typedef struct _tagEOS_Lobby_JoinLobbyCallbackInfo EOS_Lobby_JoinLobbyCallbackInfo;
typedef struct _tagEOS_Lobby_LeaveLobbyCallbackInfo EOS_Lobby_LeaveLobbyCallbackInfo;
typedef struct _tagEOS_Lobby_DestroyLobbyCallbackInfo EOS_Lobby_DestroyLobbyCallbackInfo;

namespace RN
{
	class EOSWorld;

	class EOSLobbyInfo : public Object
	{
	public:
		EOSAPI EOSLobbyInfo();
		EOSAPI ~EOSLobbyInfo();
		
		String *lobbyName;
		String *lobbyVersion;
		uint8 maximumPlayerCount;
		uint8 currentPlayerCount;
		
		EOS_HLobbyDetails lobbyHandle;
		EOS_ProductUserId ownerHandle;
		
	private:
		RNDeclareMetaAPI(EOSLobbyInfo, EOSAPI)
	};

	class EOSLobbyManager : public Object
	{
	public:
		friend class EOSWorld;

		EOSAPI ~EOSLobbyManager();
		
		EOSAPI void CreateLobby(String *lobbyName, uint8 maxUsers, std::function<void()> callback, String *lobbyVersion);
		EOSAPI void SearchLobby(std::function<void (RN::Array *)> callback);
		EOSAPI void JoinLobby(EOSLobbyInfo *lobbyInfo, std::function<void()> callback);
		EOSAPI void LeaveCurrentLobby();
		//EOSAPI void KickFromCurrentLobby(EOS_ProductUserId userHandle);
		//EOSAPI void SetCurrentLobbyAttributes(Dictionary *attributes);
		EOSAPI void ResetLobbySearchCallback();
			
	private:
		EOSAPI EOSLobbyManager(EOSWorld *world);
		
		static void LobbyOnCreateCallback(const EOS_Lobby_CreateLobbyCallbackInfo *Data);
		static void LobbyOnJoinCallback(const EOS_Lobby_JoinLobbyCallbackInfo *Data);
		static void LobbyOnLeaveCallback(const EOS_Lobby_LeaveLobbyCallbackInfo *Data);
		static void LobbyOnDestroyCallback(const EOS_Lobby_DestroyLobbyCallbackInfo *Data);
		
		static void LobbyOnSearchCallback(const EOS_LobbySearch_FindCallbackInfo *Data);
		static void LobbyOnUpdateCallback(const EOS_Lobby_UpdateLobbyCallbackInfo *Data);
		
		EOS_HLobby _lobbyInterfaceHandle;
		EOS_HLobbySearch _lobbySearchHandle;
		
		bool _isCreatingLobby;
		bool _isSearchingLobby;
		bool _isJoiningLobby;
		
		String *_createLobbyName;
		String *_createLobbyVersion;
		bool _isConnectedToLobby;
		String *_connectedLobbyID;
		bool _isConnectedLobbyOwner;
		
		std::function<void (RN::Array *)> _lobbySearchCallback;
		std::function<void()> _didJoinLobbyCallback;
			
		RNDeclareMetaAPI(EOSLobbyManager, EOSAPI)
	};
}

#endif /* defined(__RAYNE_EOSLOBBYMANAGER_H_) */
