#ifndef __RAYNE_SARELAYCLIENT_H_
#define __RAYNE_SARELAYCLIENT_H_

#include "RNSARelay.h"

namespace RN
{
	// Create, use and destroy relays on one game thread.
	// The HTTP adapter must outlive the relay.
	// Callbacks borrow arguments and may only call getters.
	// Queue changes and destruction until Update returns.
	class SARelayClient : public Object, private SlinAir::RelayClient::Events
	{
	public:
		using State = SlinAir::RelayClient::State;
		using SendResult = SlinAir::RelayClient::SendResult;
		using Member = SlinAir::RelayClient::Member;
		using Limits = SlinAir::RelayClient::Limits;

		SARELAYAPI SARelayClient(SlinAir::RelayHttp &http, const String *httpsBaseURL);
		SARELAYAPI ~SARelayClient() override;

		SARELAYAPI bool SetAccessToken(const String *token);
		SARELAYAPI bool SetMembers(const Array *accountPlayerIDs);
		SARELAYAPI virtual bool Join(const String *roomID);
		SARELAYAPI virtual void Leave();
		SARELAYAPI virtual void Update(double monotonicSeconds);

		// Channel zero is for voice. Generic datagrams use channels 1 and above.
		SARELAYAPI SendResult Broadcast(uint16 channel, const void *bytes, size_t size);
		SARELAYAPI SendResult SendTo(uint64 sessionID, uint16 channel, const void *bytes, size_t size);

		SARELAYAPI State GetState() const;
		SARELAYAPI uint64 GetSessionID() const;
		SARELAYAPI Limits GetLimits() const;
		SARELAYAPI size_t GetRoster(const Member *&members) const;

		void SetStateCallback(std::function<void(State)> callback) { _stateCallback = std::move(callback); }
		void SetRosterCallback(std::function<void(const Member *, size_t)> callback) { _rosterCallback = std::move(callback); }
		void SetPacketCallback(std::function<void(uint64, const String *, uint16, const void *, size_t)> callback) { _packetCallback = std::move(callback); }
		void SetErrorCallback(std::function<void(int, const String *)> callback) { _errorCallback = std::move(callback); }

	protected:
		static constexpr size_t MaxMembers = 63;

		virtual void HandleState(State state) {}
		virtual void HandleRoster(const Member *members, size_t count) {}
		SARELAYAPI virtual void HandlePacket(uint64 sessionID, const char *playerID, uint16 channel, const void *bytes, size_t size);

		SlinAir::RelayClient _client;

	private:
		void OnState(State state) override;
		void OnRoster(const Member *members, int count) override;
		void OnPacket(uint64_t sessionID, const char *playerID, uint16_t channel, const void *bytes, size_t size) override;
		void OnError(int status, const char *code) override;

		std::function<void(State)> _stateCallback;
		std::function<void(const Member *, size_t)> _rosterCallback;
		std::function<void(uint64, const String *, uint16, const void *, size_t)> _packetCallback;
		std::function<void(int, const String *)> _errorCallback;

		RNDeclareMetaAPI(SARelayClient, SARELAYAPI)
	};
}

#endif
