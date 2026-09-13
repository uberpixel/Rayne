//
//  RNEOSP2PClient.h
//  Rayne-EOS
//
//  Copyright 2025 by Guacam. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_EOSP2PCLIENT_H_
#define __RAYNE_EOSP2PCLIENT_H_

#include "RNEOSHost.h"
#include <set>

typedef struct _tagEOS_P2P_OnIncomingConnectionRequestInfo EOS_P2P_OnIncomingConnectionRequestInfo;
typedef struct _tagEOS_P2P_OnRemoteConnectionClosedInfo EOS_P2P_OnRemoteConnectionClosedInfo;

namespace RN
{
	class EOSP2PClient : public EOSHost
	{
	public:
		EOSAPI EOSP2PClient(bool isHost, String *socketID, EOS_ProductUserId hostProductUserID);
		EOSAPI ~EOSP2PClient() override;

		EOSAPI void Connect(EOS_ProductUserId remoteProductUserID);
		EOSAPI void Disconnect() override;
		EOSAPI void DisconnectClient(EOS_ProductUserId productUserId) override;
		EOSAPI void DisconnectClient(EOSClientID clientID);
		EOSAPI void DisconnectClientDelayed(EOSClientID clientID, float delay = 1.0f); //Using this, will not immediately force disconnect the user, leaving some time for previously sent data to arrive (like a reason for getting disconnected)
		EOSAPI void MigrateHost(EOS_ProductUserId hostProductUserId);
		
		EOSAPI bool IsHost() const { return _hostClientID != CLIENT_ID_NONE && _hostClientID == _clientID; }

	protected:
		EOSAPI void ReceivedPacketInternal(uint8 *rawData, uint32 bytesWritten, EOS_ProductUserId senderUserID, uint8 channel) final;
		EOSAPI void Update(float delta) override;
		EOSAPI virtual void HandleHostMigration(){}
		EOSAPI virtual void HandleConnectionInterrupted(EOSClientID) {}
		EOSAPI virtual bool ShouldAcceptPeer(EOS_ProductUserId productUserID) const { return true; }
		using EOSHost::HandleReliablePacketLoss;
		EOSAPI void HandleReliablePacketLoss(EOSClientID clientID) override;
		
		EOSClientID _hostClientID;
		EOS_ProductUserId _hostProductUserID;
		bool _isHostMigrationPending;

	private:
		static void OnConnectionRequestCallback(const EOS_P2P_OnIncomingConnectionRequestInfo *Data);
		static void OnConnectionClosedCallback(const EOS_P2P_OnRemoteConnectionClosedInfo *Data);

		uint64 _connectionRequestNotificationID;
		uint64 _connectionClosedNotificationID;

		bool SendConnectRequest(EOS_ProductUserId receiverID);
		Peer *BindPeerLocked(EOS_ProductUserId productUserID);
		bool TryCompleteHostMigration();
		void FinalizePeerDisconnect(EOS_ProductUserId productUserID, uint16 reason);
		
		float _connectionTimeout;
		std::set<EOS_ProductUserId> _blockedPeers;

		RNDeclareMetaAPI(EOSP2PClient, EOSAPI)
	};
} // namespace RN

#endif /* defined(__RAYNE_EOSP2PCLIENT_H_) */
