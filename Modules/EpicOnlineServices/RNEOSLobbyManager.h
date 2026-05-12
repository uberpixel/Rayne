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

struct EOS_LobbyHandle;
typedef struct EOS_LobbyHandle *EOS_HLobby;

struct EOS_LobbyDetailsHandle;
typedef struct EOS_LobbyDetailsHandle *EOS_HLobbyDetails;

struct EOS_LobbySearchHandle;
typedef struct EOS_LobbySearchHandle *EOS_HLobbySearch;

struct EOS_ProductUserIdDetails;
typedef struct EOS_ProductUserIdDetails *EOS_ProductUserId;

struct EOS_RTCHandle;
typedef struct EOS_RTCHandle *EOS_HRTC;

struct EOS_RTCAudioHandle;
typedef struct EOS_RTCAudioHandle *EOS_HRTCAudio;

typedef struct _tagEOS_Lobby_CreateLobbyCallbackInfo EOS_Lobby_CreateLobbyCallbackInfo;
typedef struct _tagEOS_LobbySearch_FindCallbackInfo EOS_LobbySearch_FindCallbackInfo;
typedef struct _tagEOS_Lobby_UpdateLobbyCallbackInfo EOS_Lobby_UpdateLobbyCallbackInfo;
typedef struct _tagEOS_Lobby_JoinLobbyCallbackInfo EOS_Lobby_JoinLobbyCallbackInfo;
typedef struct _tagEOS_Lobby_LeaveLobbyCallbackInfo EOS_Lobby_LeaveLobbyCallbackInfo;
typedef struct _tagEOS_Lobby_DestroyLobbyCallbackInfo EOS_Lobby_DestroyLobbyCallbackInfo;
typedef struct _tagEOS_Lobby_KickMemberCallbackInfo EOS_Lobby_KickMemberCallbackInfo;
typedef struct _tagEOS_Lobby_LobbyMemberStatusReceivedCallbackInfo EOS_Lobby_LobbyMemberStatusReceivedCallbackInfo;

typedef struct _tagEOS_RTCAudio_AudioBeforeSendCallbackInfo EOS_RTCAudio_AudioBeforeSendCallbackInfo;
typedef struct _tagEOS_RTCAudio_AudioBeforeRenderCallbackInfo EOS_RTCAudio_AudioBeforeRenderCallbackInfo;
typedef struct _tagEOS_RTCAudio_UpdateSendingCallbackInfo EOS_RTCAudio_UpdateSendingCallbackInfo;

typedef uint64_t EOS_NotificationId;

namespace RN
{
	class EOSWorld;
	class EOSHost;

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
		EOSAPI String *GetAttribute(const String *key) const;

	private:
		RNDeclareMetaAPI(EOSLobbyInfo, EOSAPI)
	};

	class EOSConnectedLobbyInfo : public Object
	{
	public:
		friend class EOSLobbyManager;
		
		enum class Status
		{
			Disconnected,
			Connected,
			Connecting,
			Creating,
			Disconnecting
		};
		
		EOSAPI ~EOSConnectedLobbyInfo();
		
		Status GetStatus() const { return _status; }
		
		std::vector<EOS_ProductUserId> GetRemoteClientIDs() const { return _remotePeers; }
		EOSAPI EOS_ProductUserId GetLobbyOwnerID() const;
		
		String *GetLobbyID() const { return _lobbyID; }
		
		EOSAPI void LeaveLobby();
		EOSAPI void KickFromLobby(EOS_ProductUserId userHandle);
		EOSAPI void SetLobbyAttributes(Dictionary *attributes);
		EOSAPI void SetLobbyMaxPlayers(size_t playerCount);
		
		EOSAPI void RetrievePeers();
		EOSAPI void AddRemotePeer(EOS_ProductUserId peerID);
		EOSAPI void RemoveRemotePeer(EOS_ProductUserId peerID);
		
		EOSAPI void SetAssociatedHost(EOSHost *host) { _associatedHost = host; } //Disconnect, DisconnectClient and MigrateHost will be called on this host if it is set

	private:
		EOSAPI EOSConnectedLobbyInfo();
		
		EOSHost *_associatedHost;
		
		Status _status;
		int64 _createTimestamp;
		
		EOS_HLobbyDetails _lobbyDetails;
		bool _isHost;
		
		String *_lobbyID;
		String *_lobbyName;
		String *_lobbyLevel;
		String *_lobbyVersion;
		bool _lobbyHasPassword;
		
		std::function<void(EOSResult, EOSConnectedLobbyInfo *)> _didJoinLobbyCallback;
		
		std::vector<EOS_ProductUserId> _remotePeers;
		
		EOS_NotificationId _audioBeforeRenderNotificationID;
		EOS_NotificationId _audioBeforeSendNotificationID;
		
		RNDeclareMetaAPI(EOSConnectedLobbyInfo, EOSAPI)
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
		friend class EOSConnectedLobbyInfo;

		EOSAPI ~EOSLobbyManager();

		EOSAPI void SetGlobalAudioOptions(bool voiceEnabled, bool unmixed, std::function<void(RN::String *eosUserID, RN::uint32 sampleRate, RN::uint32 channels, RN::uint32 framesCount, RN::int16 *frames, EOSConnectedLobbyInfo *connectedLobbyInfo)> audioReceivedCallback = nullptr, std::function<void(RN::uint32 sampleRate, RN::uint32 channels, RN::uint32 framesCount, RN::int16 *frames, EOSConnectedLobbyInfo *connectedLobbyInfo)> audioBeforeSendCallback = nullptr);
		EOSAPI void SetLocalPlayerMuted(bool mute);
		bool GetLocalPlayerMuted() const { return _isLocalPlayerMuted; }

		EOSAPI EOSConnectedLobbyInfo *CreateLobby(int64 createLobbyTimestamp, String *lobbyName, String *lobbyLevel, uint8 maxUsers, std::function<void(EOSResult, EOSConnectedLobbyInfo *)> callback, String *lobbyVersion, bool hasPassword, const String *lobbyIDOverride = nullptr);
		EOSAPI EOSConnectedLobbyInfo *JoinLobby(EOSLobbyInfo *lobbyInfo, std::function<void(EOSResult, EOSConnectedLobbyInfo *)> callback);
		EOSAPI void SearchLobby(bool includePrivate, bool includePublic, uint32 maxResults, std::function<void(EOSResult, RN::Array *)> callback, const RN::String *lobbyID = nullptr, RN::Array *searchFilter = nullptr, const RN::String *eosUserID = nullptr);
		EOSAPI void ResetLobbySearchCallback();
		
		std::vector<EOSConnectedLobbyInfo *> GetConnectedLobbies() const { return _connectedLobbies; }
		EOSAPI size_t GetConnectedLobbyCount() const;
		EOSAPI size_t GetConnectingLobbyCount() const;

		//TODO: Deprecated, remove once grab social lobby is merged into develop
		bool GetIsConnectedToLobby() const { return _connectedLobbies.size() > 0 && _connectedLobbies.front()->GetStatus() == EOSConnectedLobbyInfo::Status::Connected; }
		const RN::String *GetConnectedLobbyID() const { return _connectedLobbies.size() > 0 ? _connectedLobbies.front()->GetLobbyID() : nullptr; }
		void LeaveCurrentLobby() { if(_connectedLobbies.size() > 0) _connectedLobbies.front()->LeaveLobby(); }
		void KickFromCurrentLobby(EOS_ProductUserId userHandle) { if(_connectedLobbies.size() > 0) _connectedLobbies.front()->KickFromLobby(userHandle); }
		void SetCurrentLobbyAttributes(Dictionary *attributes) { if(_connectedLobbies.size() > 0) _connectedLobbies.front()->SetLobbyAttributes(attributes); }

	private:
		EOSAPI EOSLobbyManager(EOSWorld *world);
		void RemoveLobbyAudioNotifications(EOSConnectedLobbyInfo *connectedLobbyInfo, bool removeBeforeRender = true, bool removeBeforeSend = true);

		static void LobbyOnCreateCallback(const EOS_Lobby_CreateLobbyCallbackInfo *Data);
		static void LobbyOnJoinCallback(const EOS_Lobby_JoinLobbyCallbackInfo *Data);
		static void LobbyOnLeaveCallback(const EOS_Lobby_LeaveLobbyCallbackInfo *Data);
		static void LobbyOnMemberStatusReceived(const EOS_Lobby_LobbyMemberStatusReceivedCallbackInfo *Data);
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

		EOS_NotificationId _memberStatusReceivedNotificationID;

		bool _isVoiceEnabled;
		bool _isVoiceUnmixed;
		bool _isLocalPlayerMuted;

		std::vector<EOSLobbySearch *> _lobbySearches;
		std::vector<EOSConnectedLobbyInfo *> _connectedLobbies;

		std::function<void(RN::String *eosUserID, RN::uint32 sampleRate, RN::uint32 channels, RN::uint32 framesCount, RN::int16 *frames, EOSConnectedLobbyInfo *connectedLobbyInfo)> _audioReceivedCallback;
		std::function<void(RN::uint32 sampleRate, RN::uint32 channels, RN::uint32 framesCount, RN::int16 *frames, EOSConnectedLobbyInfo *connectedLobbyInfo)> _audioBeforeSendCallback;

		RNDeclareMetaAPI(EOSLobbyManager, EOSAPI)
	};
} // namespace RN

#endif /* defined(__RAYNE_EOSLOBBYMANAGER_H_) */
