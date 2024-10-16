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

struct EOS_RTCHandle;
typedef struct EOS_RTCHandle* EOS_HRTC;

struct EOS_RTCAudioHandle;
typedef struct EOS_RTCAudioHandle* EOS_HRTCAudio;

typedef struct _tagEOS_Lobby_CreateLobbyCallbackInfo EOS_Lobby_CreateLobbyCallbackInfo;
typedef struct _tagEOS_LobbySearch_FindCallbackInfo EOS_LobbySearch_FindCallbackInfo;
typedef struct _tagEOS_Lobby_UpdateLobbyCallbackInfo EOS_Lobby_UpdateLobbyCallbackInfo;
typedef struct _tagEOS_Lobby_JoinLobbyCallbackInfo EOS_Lobby_JoinLobbyCallbackInfo;
typedef struct _tagEOS_Lobby_LeaveLobbyCallbackInfo EOS_Lobby_LeaveLobbyCallbackInfo;
typedef struct _tagEOS_Lobby_DestroyLobbyCallbackInfo EOS_Lobby_DestroyLobbyCallbackInfo;
typedef struct _tagEOS_Lobby_KickMemberCallbackInfo EOS_Lobby_KickMemberCallbackInfo;

typedef struct _tagEOS_RTCAudio_AudioBeforeSendCallbackInfo EOS_RTCAudio_AudioBeforeSendCallbackInfo;
typedef struct _tagEOS_RTCAudio_AudioBeforeRenderCallbackInfo EOS_RTCAudio_AudioBeforeRenderCallbackInfo;
typedef struct _tagEOS_RTCAudio_UpdateSendingCallbackInfo EOS_RTCAudio_UpdateSendingCallbackInfo;

typedef uint64_t EOS_NotificationId;

namespace RN
{
	class EOSWorld;

	class EOSLobbyInfo : public Object
	{
	public:
		EOSAPI EOSLobbyInfo();
		EOSAPI ~EOSLobbyInfo();
		
		String *lobbyName;
		String *lobbyLevel;
		String *lobbyVersion;
		uint8 maximumPlayerCount;
		uint8 currentPlayerCount;
		int64 createTimestamp;
		bool hasPassword;
		
		EOS_HLobbyDetails lobbyHandle;
		EOS_ProductUserId ownerHandle;
		
		EOSAPI const String *GetDescription() const override;
		
	private:
		RNDeclareMetaAPI(EOSLobbyInfo, EOSAPI)
	};

	class EOSLobbySearchParameter : public Object
	{
	public:
		friend class EOSLobbyManager;
		enum Comparator
		{
			ComparatorEqual,
			ComparatorNotEqual,
			ComparatorNumGreaterThan,
			ComparatorNumGreaterThanOrEqual,
			ComparatorNumLessThan,
			ComparatorNumLessThanOrEqual,
			ComparatorNumDistance,
			ComparatorStringAnyOf,
			ComparatorStringNotAnyOf
		};
		
		EOSAPI ~EOSLobbySearchParameter();
		
	protected:
		EOSAPI EOSLobbySearchParameter(String *name, Comparator comparator);
		
	private:
		String *_name;
		Comparator _comparator;
		
		RNDeclareMetaAPI(EOSLobbySearchParameter, EOSAPI)
	};

	class EOSLobbySearchParameterString : public EOSLobbySearchParameter
	{
	public:
		friend class EOSLobbyManager;
		EOSAPI EOSLobbySearchParameterString(String *name, String *content, Comparator comparator);
		EOSAPI ~EOSLobbySearchParameterString();
		
	private:
		String *_content;
		
		RNDeclareMetaAPI(EOSLobbySearchParameterString, EOSAPI)
	};

	class EOSLobbyManager;
	struct EOSLobbySearch
	{
		EOS_HLobbySearch handle;
		std::function<void(EOSResult, RN::Array *)> callback;
	};

	class EOSLobbyManager : public Object
	{
	public:
		friend class EOSWorld;

		EOSAPI ~EOSLobbyManager();
		
		EOSAPI void SetGlobalAudioOptions(bool voiceEnabled, bool unmixed, std::function<void(RN::String *eosUserID, RN::uint32 sampleRate, RN::uint32 channels, RN::uint32 framesCount, RN::int16 *frames)> audioReceivedCallback = nullptr, std::function<void(RN::uint32 sampleRate, RN::uint32 channels, RN::uint32 framesCount, RN::int16 *frames)> audioBeforeSendCallback = nullptr);
		EOSAPI void SetLocalPlayerMuted(bool mute);
		bool GetLocalPlayerMuted() const {return _isLocalPlayerMuted;}
		
		EOSAPI void CreateLobby(int64 createLobbyTimestamp, String *lobbyName, String *lobbyLevel, uint8 maxUsers, std::function<void(EOSResult)> callback, String *lobbyVersion, bool hasPassword, const String *lobbyIDOverride = nullptr);
		EOSAPI void SearchLobby(bool includePrivate, bool includePublic, uint32 maxResults, std::function<void(EOSResult, RN::Array *)> callback, const RN::String *lobbyID = nullptr, RN::Array *searchFilter = nullptr);
		EOSAPI void JoinLobby(EOSLobbyInfo *lobbyInfo, std::function<void(EOSResult)> callback);
		EOSAPI void LeaveCurrentLobby();
		EOSAPI void KickFromCurrentLobby(EOS_ProductUserId userHandle);
		EOSAPI void SetCurrentLobbyAttributes(Dictionary *attributes);
		EOSAPI void ResetLobbySearchCallback();
		
		bool GetIsConnectedToLobby() const { return _isConnectedToLobby; }
		const RN::String *GetConnectedLobbyID() const { return _connectedLobbyID; }
			
	private:
		EOSAPI EOSLobbyManager(EOSWorld *world);
		
		static void LobbyOnCreateCallback(const EOS_Lobby_CreateLobbyCallbackInfo *Data);
		static void LobbyOnJoinCallback(const EOS_Lobby_JoinLobbyCallbackInfo *Data);
		static void LobbyOnLeaveCallback(const EOS_Lobby_LeaveLobbyCallbackInfo *Data);
		static void LobbyOnDestroyCallback(const EOS_Lobby_DestroyLobbyCallbackInfo *Data);
		static void LobbyOnKickMemberCallback(const EOS_Lobby_KickMemberCallbackInfo *Data);
		
		static void LobbyOnSearchCallback(const EOS_LobbySearch_FindCallbackInfo *Data);
		static void LobbyOnUpdateCallback(const EOS_Lobby_UpdateLobbyCallbackInfo *Data);
		
		static void LobbyAudioOnBeforeSendCallback(const EOS_RTCAudio_AudioBeforeSendCallbackInfo *Data);
		static void LobbyAudioOnBeforeRenderCallback(const EOS_RTCAudio_AudioBeforeRenderCallbackInfo *Data);
		static void LobbyAudioOnUpdateSendingCallback(const EOS_RTCAudio_UpdateSendingCallbackInfo *Data);
		
		EOS_HLobby _lobbyInterfaceHandle;
		
		EOS_HRTC _rtcInterfaceHandle;
		EOS_HRTCAudio _rtcAudioInterfaceHandle;
		
		EOS_NotificationId _currentAudioBeforeRenderNotificationID;
		EOS_NotificationId _currentAudioBeforeSendNotificationID;
		
		bool _isCreatingLobby;
		bool _isJoiningLobby;
		
		String *_createLobbyName;
		String *_createLobbyLevel;
		String *_createLobbyVersion;
		int64 _createLobbyTimestamp;
		bool _createLobbyHasPassword;
		bool _isConnectedToLobby;
		String *_connectedLobbyID;
		bool _isConnectedLobbyOwner;
		
		bool _isVoiceEnabled;
		bool _isVoiceUnmixed;
		bool _isLocalPlayerMuted;
		
		std::vector<EOSLobbySearch*> _lobbySearches;
		
		std::function<void(EOSResult)> _didJoinLobbyCallback;
		
		std::function<void(RN::String *eosUserID, RN::uint32 sampleRate, RN::uint32 channels, RN::uint32 framesCount, RN::int16 *frames)> _audioReceivedCallback;
		std::function<void(RN::uint32 sampleRate, RN::uint32 channels, RN::uint32 framesCount, RN::int16 *frames)> _audioBeforeSendCallback;
			
		RNDeclareMetaAPI(EOSLobbyManager, EOSAPI)
	};
}

#endif /* defined(__RAYNE_EOSLOBBYMANAGER_H_) */
