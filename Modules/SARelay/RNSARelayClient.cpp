#include "RNSARelayClient.h"

namespace RN
{
	RNDefineMeta(SARelayClient, Object)

	SARelayClient::SARelayClient(SlinAir::RelayHttp &http, const String *httpsBaseURL) :
		_client(http, *this, httpsBaseURL ? httpsBaseURL->GetUTF8String() : "")
	{}

	SARelayClient::~SARelayClient() {}

	bool SARelayClient::SetAccessToken(const String *token)
	{
		return _client.SetAccessToken(token ? token->GetUTF8String() : "");
	}

	bool SARelayClient::SetMembers(const Array *accountPlayerIDs)
	{
		const size_t count = accountPlayerIDs ? accountPlayerIDs->GetCount() : 0;
		if(count > MaxMembers) return false;

		const char *members[MaxMembers];
		for(size_t i = 0; i < count; i++)
		{
			const String *playerID = accountPlayerIDs->GetObjectAtIndex(i)->Downcast<String>();
			if(!playerID) return false;
			members[i] = playerID->GetUTF8String();
		}

		return _client.SetMembers(members, static_cast<int>(count));
	}

	bool SARelayClient::Join(const String *roomID)
	{
		return roomID && _client.Join(roomID->GetUTF8String());
	}

	void SARelayClient::Leave()
	{
		_client.Leave();
	}

	void SARelayClient::Update(double monotonicSeconds)
	{
		_client.Update(monotonicSeconds);
	}

	SARelayClient::SendResult SARelayClient::Broadcast(uint16 channel, const void *bytes, size_t size)
	{
		return channel ? _client.Broadcast(channel, bytes, size) : SendResult::InvalidPayload;
	}

	SARelayClient::SendResult SARelayClient::SendTo(uint64 sessionID, uint16 channel, const void *bytes, size_t size)
	{
		return channel ? _client.SendTo(sessionID, channel, bytes, size) : SendResult::InvalidPayload;
	}

	SARelayClient::State SARelayClient::GetState() const { return _client.GetState(); }
	uint64 SARelayClient::GetSessionID() const { return _client.GetSessionId(); }
	SARelayClient::Limits SARelayClient::GetLimits() const { return _client.GetLimits(); }
	size_t SARelayClient::GetRoster(const Member *&members) const { return _client.GetRoster(members); }

	void SARelayClient::OnState(State state)
	{
		HandleState(state);
		if(_stateCallback) _stateCallback(state);
	}

	void SARelayClient::OnRoster(const Member *members, int count)
	{
		HandleRoster(members, count);
		if(_rosterCallback) _rosterCallback(members, count);
	}

	void SARelayClient::OnPacket(uint64_t sessionID, const char *playerID, uint16_t channel, const void *bytes, size_t size)
	{
		HandlePacket(sessionID, playerID, channel, bytes, size);
	}

	void SARelayClient::OnError(int status, const char *code)
	{
		if(_errorCallback) _errorCallback(status, String::WithString(code));
	}

	void SARelayClient::HandlePacket(uint64 sessionID, const char *playerID, uint16 channel, const void *bytes, size_t size)
	{
		if(channel && _packetCallback) _packetCallback(sessionID, String::WithString(playerID), channel, bytes, size);
	}
}
