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

	EOSWorld::EOSWorld() : _hosts(new Array()), _isLoggedIn(false), _loggedInUserID(nullptr), _lobbyManager(nullptr)
	{
		RN_ASSERT(!_instance, "There already is an EOSWorld!");

#if RN_PLATFORM_ANDROID
		EOS_InitializeOptions SDKOptions = { 0 };
		SDKOptions.ApiVersion = EOS_INITIALIZE_API_LATEST;
		SDKOptions.ProductName = "Concealed";
		SDKOptions.ProductVersion = "0.1";

		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();

		static EOS_Android_InitializeOptions JNIOptions = { 0 };
		JNIOptions.ApiVersion = EOS_ANDROID_INITIALIZEOPTIONS_API_LATEST;
		JNIOptions.VM = app->activity->vm;

		JNIOptions.OptionalInternalDirectory = app->activity->internalDataPath;
		JNIOptions.OptionalExternalDirectory = app->activity->externalDataPath;

		SDKOptions.SystemInitializeOptions = &JNIOptions;

		if(EOS_Initialize(&SDKOptions) != EOS_EResult::EOS_Success)
		{
			RNDebug("Failed initializing EOS.");
		}
#else
		EOS_InitializeOptions SDKOptions = { 0 };
		SDKOptions.ApiVersion = EOS_INITIALIZE_API_LATEST;
		SDKOptions.ProductName = "Concealed";
		SDKOptions.ProductVersion = "0.1";

		if(EOS_Initialize(&SDKOptions) != EOS_EResult::EOS_Success)
		{
			RNDebug("Failed initializing EOS.");
		}
#endif
		
		EOS_Logging_SetCallback(LoggingCallback);
		EOS_Logging_SetLogLevel(EOS_ELogCategory::EOS_LC_ALL_CATEGORIES, EOS_ELogLevel::EOS_LOG_VeryVerbose);
		
		EOS_Platform_Options platformOptions = {0};
		platformOptions.ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;
		platformOptions.ProductId = "50a04c8e24e345dfa61821175da71cd2";
		platformOptions.SandboxId = "cd526f483ce44807a62c25073965696f";
		platformOptions.ClientCredentials.ClientId = "xyza7891wae50pvCMplljZZnTMNoLirF";
		platformOptions.ClientCredentials.ClientSecret = "IHxI1XPK7KmUVjNIjHRmjFm63oRFBhAC3W9RtQN9M+s";
		platformOptions.DeploymentId = "c431e7d730764d2991b5e96a0bfe13d4";
		platformOptions.bIsServer = false;
		platformOptions.EncryptionKey = nullptr;
		platformOptions.Flags = EOS_PF_DISABLE_OVERLAY; //Social or IAP overlay not made for VR is quite useless in VR...
		platformOptions.CacheDirectory = nullptr;
		platformOptions.TickBudgetInMilliseconds = 0; //Do all work, no matter how long
		
		_platformHandle = EOS_Platform_Create(&platformOptions);
		
		_connectInterfaceHandle = EOS_Platform_GetConnectInterface(_platformHandle);
		_p2pInterfaceHandle = EOS_Platform_GetP2PInterface(_platformHandle);

#if !RN_PLATFORM_ANDROID
		LoginUser();
#endif

		_instance = this;
	}
		
	EOSWorld::~EOSWorld()
	{
		_hosts->Release();
		EOS_Platform_Release(_platformHandle);

		_instance = nullptr;
		EOS_Shutdown();

		SafeRelease(_loginToken);
	}

	void EOSWorld::Update(float delta)
	{
		EOS_Platform_Tick(_platformHandle);
		
		_hosts->Enumerate<EOSHost>([&](EOSHost *host, size_t index, bool &stop) {
			host->Update(delta);
		});
	}

	void EOSWorld::StartOculusLogin(String *userID, String *nonce)
	{
		_loginToken = RNSTR(userID << "|" << nonce)->Retain();
		LoginUser();
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
		EOS_Connect_Credentials connectCredentials = {};
		connectCredentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;

#if RN_PLATFORM_ANDROID
		connectCredentials.Type = EOS_EExternalCredentialType::EOS_ECT_OCULUS_USERID_NONCE;
		connectCredentials.Token = _loginToken->GetUTF8String();
#else
		connectCredentials.Type = EOS_EExternalCredentialType::EOS_ECT_DEVICEID_ACCESS_TOKEN;
#endif
		
		EOS_Connect_UserLoginInfo userInfo;
		userInfo.ApiVersion = EOS_CONNECT_USERLOGININFO_API_LATEST;
		userInfo.DisplayName = "Slin";
		
		EOS_Connect_LoginOptions connectOptions = {0};
		connectOptions.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
		connectOptions.Credentials = &connectCredentials;
		connectOptions.UserLoginInfo = &userInfo;

		EOS_Connect_Login(_connectInterfaceHandle, &connectOptions, this, ConnectOnLoginCallback);
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
			RNDebug("Succesful login");

			eosWorld->_isLoggedIn = true;
			eosWorld->_loggedInUserID = Data->LocalUserId;
		}
		else if(Data->ResultCode == EOS_EResult::EOS_InvalidUser)
		{
#if RN_PLATFORM_ANDROID
			EOS_Connect_CreateUserOptions createUserOptions = {0};
			createUserOptions.ApiVersion = EOS_CONNECT_CREATEUSER_API_LATEST;
			createUserOptions.ContinuanceToken = Data->ContinuanceToken;
			EOS_Connect_CreateUser(eosWorld->_connectInterfaceHandle, &createUserOptions, eosWorld, ConnectOnCreateUserCallback);
#endif
		}
		else if(Data->ResultCode == EOS_EResult::EOS_NotFound)
		{
#if RN_PLATFORM_ANDROID
#else
			RNDebug("No credentials found, creating device ID...");
			EOSWorld *eosWorld = static_cast<EOSWorld*>(Data->ClientData);
			eosWorld->CreateDeviceID();
#endif
		}
		else
		{
			RNDebug("Login failed");
		}
	}
}