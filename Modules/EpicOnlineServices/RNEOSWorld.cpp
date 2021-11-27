//
//  RNEOSWorld.cpp
//  Rayne-EOS
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNEOSWorld.h"

#if RN_PLATFORM_ANDROID
#include "Android/eos_Android_base.h"
#include "Android/eos_android.h"
#endif

#include "eos_platform_prereqs.h"
#include "eos_sdk.h"
#include "eos_logging.h"
#include "eos_common.h"
#include "eos_auth.h"
#include "eos_auth_types.h"
#include "eos_connect.h"
#include "eos_connect_types.h"
#include "eos_lobby.h"
#include "eos_lobby_types.h"

#if !RN_PLATFORM_WINDOWS
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#endif

namespace RN
{
	RNDefineMeta(EOSWorld, SceneAttachment)

	EOSWorld *EOSWorld::_instance = nullptr;

	EOSWorld* EOSWorld::GetInstance()
	{
		return _instance;
	}

	EOSWorld::EOSWorld(String *productName, String *productVersion, String *productID, String *sandboxID, String *deploymentID, String *clientID, String *clientSecret, std::function<void(std::function<void(String *, String *, EOSAuthServiceType)>)> externalLoginCallback) : _hosts(new Array()), _externalLoginCallback(nullptr), _loginState(LoginStateIsNotLoggedIn), _loggedInUserID(nullptr), _lobbyManager(nullptr)
	{
		RN_ASSERT(!_instance, "There already is an EOSWorld!");

		if(externalLoginCallback)
		{
			_externalLoginCallback = std::move(externalLoginCallback);
		}

		EOS_InitializeOptions SDKOptions = { 0 };
		SDKOptions.ApiVersion = EOS_INITIALIZE_API_LATEST;
		SDKOptions.ProductName = productName->GetUTF8String();
		SDKOptions.ProductVersion = productVersion->GetUTF8String();
		
#if RN_PLATFORM_ANDROID
		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();

		static EOS_Android_InitializeOptions JNIOptions = { 0 };
		JNIOptions.ApiVersion = EOS_ANDROID_INITIALIZEOPTIONS_API_LATEST;

		JNIOptions.OptionalInternalDirectory = app->activity->internalDataPath;
		JNIOptions.OptionalExternalDirectory = app->activity->externalDataPath;

		SDKOptions.SystemInitializeOptions = &JNIOptions;
#endif

		EOS_EResult result = EOS_Initialize(&SDKOptions);
		if(result != EOS_EResult::EOS_Success)
		{
			RNDebug("Failed initializing EOS.");
			return;
		}

		EOS_Logging_SetCallback(LoggingCallback);
		EOS_Logging_SetLogLevel(EOS_ELogCategory::EOS_LC_ALL_CATEGORIES, /*EOS_ELogLevel::EOS_LOG_Warning*/EOS_ELogLevel::EOS_LOG_VeryVerbose);

		EOS_Platform_Options platformOptions = {0};
		platformOptions.ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;
		platformOptions.ProductId = productID->GetUTF8String();
		platformOptions.SandboxId = sandboxID->GetUTF8String();
		platformOptions.ClientCredentials.ClientId = clientID->GetUTF8String();
		platformOptions.ClientCredentials.ClientSecret = clientSecret->GetUTF8String();
		platformOptions.DeploymentId = deploymentID->GetUTF8String();
		platformOptions.bIsServer = false;
		platformOptions.EncryptionKey = nullptr;
		platformOptions.Flags = EOS_PF_DISABLE_OVERLAY; //Social or IAP overlay not made for VR is quite useless in VR...
		platformOptions.CacheDirectory = nullptr;
		platformOptions.TickBudgetInMilliseconds = 0; //Do all work, no matter how long
		
		_platformHandle = EOS_Platform_Create(&platformOptions);
		
		_connectInterfaceHandle = EOS_Platform_GetConnectInterface(_platformHandle);
		_p2pInterfaceHandle = EOS_Platform_GetP2PInterface(_platformHandle);
		
		EOS_Connect_AddNotifyAuthExpirationOptions authExpirationOptions = {0};
		authExpirationOptions.ApiVersion = EOS_CONNECT_ADDNOTIFYAUTHEXPIRATION_API_LATEST;
		EOS_Connect_AddNotifyAuthExpiration(_connectInterfaceHandle, &authExpirationOptions, this, ConnectOnAuthExpirationCallback);
		
		EOS_Connect_AddNotifyLoginStatusChangedOptions loginStatusChangedOptions = {0};
		loginStatusChangedOptions.ApiVersion = EOS_CONNECT_ADDNOTIFYLOGINSTATUSCHANGED_API_LATEST;
		EOS_Connect_AddNotifyLoginStatusChanged(_connectInterfaceHandle, &loginStatusChangedOptions, this, ConnectOnLoginStatusChangedCallback);

		_instance = this;
	}
		
	EOSWorld::~EOSWorld()
	{
		_hosts->Release();
		EOS_Platform_Release(_platformHandle);

		_instance = nullptr;
		EOS_Shutdown();
	}

	void EOSWorld::Update(float delta)
	{
		EOS_Platform_Tick(_platformHandle);
		
		_hosts->Enumerate<EOSHost>([&](EOSHost *host, size_t index, bool &stop) {
			host->Update(delta);
		});
	}

	void EOSWorld::AddHost(EOSHost *host)
	{
		_hosts->AddObject(host);
	}

	void EOSWorld::RemoveHost(EOSHost *host)
	{
		_hosts->RemoveObject(host);
	}

	EOSLobbyManager *EOSWorld::GetLobbyManager()
	{
		if(!_lobbyManager)
		{
			_lobbyManager = new EOSLobbyManager(this);
		}
		
		return _lobbyManager;
	}

	double EOSWorld::Ping(String *ip, size_t repetitions)
	{
/*#if !RN_PLATFORM_WINDOWS
		RNDebug("Pinging " << ip << ".");
		
		struct PingPacket
		{
			struct icmp header;
			char padding[64 - sizeof(struct icmp)]; //Extra padding to get to minimum packet size
		};
		
		struct sockaddr_in addr;
		PingPacket packet;
		
		int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_ICMP);
		if(sock < 0)
		{
			RNDebug("Ping failed opening ICMP socket! Need to be root user!?");
		}
		
		struct timeval tv_out;
		tv_out.tv_sec = 1;
		tv_out.tv_usec = 0;
		
		// setting timeout of recv setting
//		int ttl_val = 64;
//		setsockopt(sock, SOL_IP, IP_TTL, &ttl_val, sizeof(ttl_val));
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_out, sizeof(tv_out));
		
		std::vector<double> pingTimes;
		pingTimes.reserve(repetitions);

		// Send the packet
		for(int i = 0; i < repetitions; i++)
		{
			RNDebug("Ping " << i);
			
			EOSAddress address;
			EOS_address_set_host(&address, ip->GetUTF8String());
			
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = address.host;

			// Initialize the ICMP header
			memset(&packet.header, 0, sizeof(packet));
			packet.header.icmp_type = ICMP_ECHO;
			packet.header.icmp_code = 0;
			packet.header.icmp_hun.ih_idseq.icd_id = 1337;
			packet.header.icmp_hun.ih_idseq.icd_seq = i+1;
			
			//Calculate checksum
			int packetLength = sizeof(packet);
			unsigned short *buf = reinterpret_cast<unsigned short*>(&packet);
			unsigned int checksum = 0;
			for(checksum = 0; packetLength > 1; packetLength -= 2)
			{
				checksum += *buf++;
			}
			if(packetLength == 1)
			{
				checksum += *(unsigned char*)buf;
			}
			checksum = (checksum >> 16) + (checksum & 0xFFFF);
			checksum += (checksum >> 16);
			packet.header.icmp_cksum = ~checksum;
			
			struct timespec startTime;
			struct timespec endTime;
			clock_gettime(CLOCK_MONOTONIC, &startTime);
			if(sendto(sock, &packet, sizeof(packet), 0, (struct sockaddr*) &addr, sizeof(addr)) < 0)
			{
				RNDebug("Ping failed sending!");
				continue;
			}
			
			//Waiting for answer to ping
			sockaddr_in r_addr;
			socklen_t addr_len = sizeof(r_addr);

			RNDebug("Ping waiting for response!");
			while(1)
			{
				if(recvfrom(sock, &packet, sizeof(packet), 0, (struct sockaddr*) &r_addr, &addr_len) <= 0)
				{
					RNDebug("Ping packet receive failed!");
					break;
				}
				else
				{
					//The 69 here seems to be a kernel bug on some systems (including macOS): https://blog.benjojo.co.uk/post/linux-icmp-type-69
					if((packet.header.icmp_type == ICMP_ECHOREPLY || packet.header.icmp_type == 69) && packet.header.icmp_code == 0) //It is an echo response!
					{
						clock_gettime(CLOCK_MONOTONIC, &endTime);
						double timeElapsed = (endTime.tv_sec- startTime.tv_sec) * 1000.0 + ((double)(endTime.tv_nsec - startTime.tv_nsec))/1000000.0;
						pingTimes.push_back(timeElapsed);
						RNDebug("Ping success: " << timeElapsed);
						break;
					}
					else
					{
						RNDebug("Ping error: Packet received with ICMP type " << packet.header.icmp_type << " and code " << packet.header.icmp_code);
					}
				}
			}
			
			usleep(100);
		}
		
		std::sort(pingTimes.begin(), pingTimes.end());*/
		
		/*double averagePing = 0.0;
		int filterSize = pingTimes.size() / 4;
		for(int i = filterSize; i < pingTimes.size() - filterSize; i++)
		{
			RNDebug("Ping " << pingTimes[i]);
			averagePing += pingTimes[i];
		}
		averagePing /= pingTimes.size() - 2 * filterSize;
		
		RNDebug("Pinging Finished with average of " << averagePing << "ms");*/
		
/*		double lowestPing = pingTimes.size() > 0?pingTimes[0]:10000.0;;
		RNDebug("Pinging Finished with lowest of " << lowestPing << "ms");
		
		return lowestPing;
#endif*/
		
		return 0.0;
	}

	void EOSWorld::CreateDeviceID()
	{
		EOS_Connect_CreateDeviceIdOptions deviceIdOptions = {0};
		deviceIdOptions.ApiVersion = EOS_CONNECT_CREATEDEVICEID_API_LATEST;
		deviceIdOptions.DeviceModel = "Mac";
		EOS_Connect_CreateDeviceId(_connectInterfaceHandle, &deviceIdOptions, this, ConnectOnCreateDeviceIDCallback);
	}

	void EOSWorld::LoginUser()
	{
		RNDebug("Start user login");
		if(_loginState == LoginStateIsLoggingIn || _loginState == LoginStateIsLoggedIn) return;
		
		RNDebug("Start user login for real");
		_loginState = LoginStateIsLoggingIn;
		
		std::function<void(String *, String *, EOSAuthServiceType)> loginCallback = [&](String *userName, String *loginToken, EOSAuthServiceType serviceType){
			
			EOS_Connect_Credentials connectCredentials = {};
			connectCredentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;

			if(loginToken && serviceType != EOSAuthServiceTypeNone)
			{
				if(serviceType == EOSAuthServiceTypeOculus)
				{
					connectCredentials.Type = EOS_EExternalCredentialType::EOS_ECT_OCULUS_USERID_NONCE;
				}
				
				connectCredentials.Token = loginToken->GetUTF8String();
			}
			else
			{
				connectCredentials.Type = EOS_EExternalCredentialType::EOS_ECT_DEVICEID_ACCESS_TOKEN;
			}
			
			EOS_Connect_UserLoginInfo userInfo;
			userInfo.ApiVersion = EOS_CONNECT_USERLOGININFO_API_LATEST;
			if(userName)
			{
				userInfo.DisplayName = userName->GetUTF8String();
			}
			else
			{
				userInfo.DisplayName = "__NO_NAME__";
			}
			
			EOS_Connect_LoginOptions connectOptions = {0};
			connectOptions.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
			connectOptions.Credentials = &connectCredentials;
			connectOptions.UserLoginInfo = &userInfo;

			RNDebug("Now logging in");
			EOS_Connect_Login(_connectInterfaceHandle, &connectOptions, this, ConnectOnLoginCallback);
		};
		
		if(_externalLoginCallback)
		{
			_externalLoginCallback(loginCallback);
		}
		else
		{
			loginCallback(nullptr, nullptr, EOSAuthServiceTypeNone);
		}
	}

	void EOSWorld::LoggingCallback(const EOS_LogMessage *Message)
	{
		RNDebug(Message->Message);
	}

	void EOSWorld::ConnectOnCreateDeviceIDCallback(const EOS_Connect_CreateDeviceIdCallbackInfo *Data)
	{
		if(Data->ResultCode == EOS_EResult::EOS_Success)
		{
			RNDebug("Succesfully created device ID");
			
			EOSWorld *eosWorld = static_cast<EOSWorld*>(Data->ClientData);
			eosWorld->_loginState = LoginStateIsLoggingInNoDeviceID;
			eosWorld->LoginUser();
		}
		else
		{
			RNDebug("Failed creating device ID");
		}
	}

	void EOSWorld::ConnectOnCreateUserCallback(const EOS_Connect_CreateUserCallbackInfo *Data)
	{
		if(Data->ResultCode == EOS_EResult::EOS_Success)
		{
			RNDebug("Succesfully created user");

			EOSWorld *eosWorld = static_cast<EOSWorld*>(Data->ClientData);
			eosWorld->_loginState = LoginStateIsLoggingInNoUser;
			eosWorld->LoginUser();
		}
		else
		{
			RNDebug("Failed creating user");
		}
	}

	void EOSWorld::ConnectOnLoginCallback(const EOS_Connect_LoginCallbackInfo *Data)
	{
		EOSWorld *eosWorld = static_cast<EOSWorld*>(Data->ClientData);
		if(Data->ResultCode == EOS_EResult::EOS_Success)
		{
			RNDebug("Successful login");

			eosWorld->_loginState = LoginStateIsLoggedIn;
			eosWorld->_loggedInUserID = Data->LocalUserId;
		}
		else if(Data->ResultCode == EOS_EResult::EOS_InvalidUser)
		{
			RNDebug("Failed login, invalid user, trying to create a new one");
#if RN_PLATFORM_ANDROID
			EOS_Connect_CreateUserOptions createUserOptions = {0};
			createUserOptions.ApiVersion = EOS_CONNECT_CREATEUSER_API_LATEST;
			createUserOptions.ContinuanceToken = Data->ContinuanceToken;
			EOS_Connect_CreateUser(eosWorld->_connectInterfaceHandle, &createUserOptions, eosWorld, ConnectOnCreateUserCallback);
#else
			eosWorld->_loginState = LoginStateLoginFailed;
#endif
		}
		else if(Data->ResultCode == EOS_EResult::EOS_NotFound)
		{
			RNDebug("No credentials found, creating device ID...");
			EOSWorld *eosWorld = static_cast<EOSWorld*>(Data->ClientData);
			eosWorld->CreateDeviceID();
		}
		else
		{
			RNDebug("Login failed");
			eosWorld->_loginState = LoginStateLoginFailed;
		}
	}

	void EOSWorld::ConnectOnAuthExpirationCallback(const EOS_Connect_AuthExpirationCallbackInfo *Data)
	{
		RNDebug("EOS auth is about to expire, starting renew process");
		EOSWorld *eosWorld = static_cast<EOSWorld*>(Data->ClientData);
		eosWorld->_loginState = LoginStateLoginExpired;
		eosWorld->LoginUser();
	}
	
	void EOSWorld::ConnectOnLoginStatusChangedCallback(const EOS_Connect_LoginStatusChangedCallbackInfo *Data)
	{
		switch(Data->CurrentStatus)
		{
			case EOS_ELoginStatus::EOS_LS_NotLoggedIn:
			{
				RNDebug("EOS not logged in");
				break;
			}
			case EOS_ELoginStatus::EOS_LS_UsingLocalProfile:
			{
				RNDebug("EOS not logged in, using local profile");
				break;
			}
			case EOS_ELoginStatus::EOS_LS_LoggedIn:
			{
				RNDebug("EOS logged in");
				break;
			}
		}
	}
}
