//
//  RNEOSWorld.h
//  Rayne-EOS
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_EOSWORLD_H_
#define __RAYNE_EOSWORLD_H_

#include "RNEOS.h"
#include "RNEOSLobbyManager.h"
#include "RNEOSClient.h"
#include "RNEOSServer.h"

struct EOS_PlatformHandle;
typedef struct EOS_PlatformHandle* EOS_HPlatform;

struct EOS_ConnectHandle;
typedef struct EOS_ConnectHandle* EOS_HConnect;

struct EOS_P2PHandle;
typedef struct EOS_P2PHandle* EOS_HP2P;

struct EOS_ProductUserIdDetails;
typedef struct EOS_ProductUserIdDetails* EOS_ProductUserId;

typedef struct _tagEOS_LogMessage EOS_LogMessage;

typedef struct _tagEOS_Connect_LoginCallbackInfo EOS_Connect_LoginCallbackInfo;
typedef struct _tagEOS_Connect_CreateDeviceIdCallbackInfo EOS_Connect_CreateDeviceIdCallbackInfo;
typedef struct _tagEOS_Connect_CreateUserCallbackInfo EOS_Connect_CreateUserCallbackInfo;
typedef struct _tagEOS_Connect_AuthExpirationCallbackInfo EOS_Connect_AuthExpirationCallbackInfo;

namespace RN
{
	enum EOSAuthServiceType
	{
		EOSAuthServiceTypeNone,
		EOSAuthServiceTypeOculus
	};

	class EOSWorld : public SceneAttachment
	{
	public:
		EOSAPI static EOSWorld *GetInstance();

		EOSAPI EOSWorld(String *productName, String *productVersion, String *productID, String *sandboxID, String *deploymentID, String *clientID, String *clientSecret, std::function<void(std::function<void(String *, String *, EOSAuthServiceType)>)> externalLoginCallback);
		EOSAPI ~EOSWorld() override;
		
		EOSAPI void AddHost(EOSHost *host);
		EOSAPI void RemoveHost(EOSHost *host);
		
		EOSAPI EOSLobbyManager *GetLobbyManager();
		EOSAPI EOS_HPlatform GetPlatformHandle() const { return _platformHandle; }
		EOSAPI EOS_HP2P GetP2PHandle() const { return _p2pInterfaceHandle; }
		EOSAPI EOS_ProductUserId GetUserID() const { return _loggedInUserID; }
		EOSAPI bool GetIsLoggedIn() const { return _isLoggedIn; }
		EOSAPI void LoginUser();
		
		EOSAPI double Ping(String *address, size_t repetitions);

	protected:
		void Update(float delta) override;
			
	private:
		static void LoggingCallback(const EOS_LogMessage *Message);
		static void ConnectOnCreateDeviceIDCallback(const EOS_Connect_CreateDeviceIdCallbackInfo *Data);
		static void ConnectOnCreateUserCallback(const EOS_Connect_CreateUserCallbackInfo *Data);
		static void ConnectOnLoginCallback(const EOS_Connect_LoginCallbackInfo *Data);
		static void ConnectOnAuthExpirationCallback(const EOS_Connect_AuthExpirationCallbackInfo *Data);
		
		void CreateDeviceID();
		
		static EOSWorld *_instance;
		Array *_hosts;
		
		std::function<void(std::function<void(String *, String *, EOSAuthServiceType)>)> _externalLoginCallback;

		bool _isLoggedIn;
		EOS_ProductUserId _loggedInUserID;
		
		EOS_HPlatform _platformHandle;
		EOS_HConnect _connectInterfaceHandle;
		EOS_HP2P _p2pInterfaceHandle;
		
		EOSLobbyManager *_lobbyManager;
			
		RNDeclareMetaAPI(EOSWorld, EOSAPI)
	};
}

#endif /* defined(__RAYNE_EOSWORLD_H_) */
