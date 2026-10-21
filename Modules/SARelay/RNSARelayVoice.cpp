#include "RNSARelayVoice.h"
#include <opus.h>

namespace RN
{
	RNDefineMeta(SARelayVoice, SARelayClient)

	bool SARelayVoice::CaptureState::SetEnabled(bool enabled)
	{
		LockGuard<Lockable> lock(_lock);
		if(_enabled == enabled) return false;

		_enabled = enabled;
		_chunks.clear();
		_frames = 0;
		_formatChanged = false;
		return true;
	}

	void SARelayVoice::CaptureState::Reset()
	{
		LockGuard<Lockable> lock(_lock);
		_chunks.clear();
		_frames = 0;
		_formatChanged = false;
	}

	void SARelayVoice::CaptureState::Enqueue(uint32 sampleRate, uint32 channels, uint32 frames, const float *samples)
	{
		if(!samples || sampleRate < 8000 || sampleRate > 192000 || !channels || channels > 8 || !frames || frames > sampleRate / 5) return;

		LockGuard<Lockable> lock(_lock);
		if(!_enabled) return;

		if(_sampleRate != sampleRate || _channels != channels)
		{
			_chunks.clear();
			_frames = 0;
			_sampleRate = sampleRate;
			_channels = channels;
			_formatChanged = true;
		}

		const size_t maxFrames = size_t(sampleRate) * CaptureFrames / SampleRate;
		const size_t retainedFrames = std::min(size_t(frames), maxFrames);
		while(_frames + retainedFrames > maxFrames)
		{
			auto &oldest = _chunks.front();
			const size_t discarded = std::min(_frames + retainedFrames - maxFrames, oldest.samples.size() / channels);
			oldest.samples.erase(oldest.samples.begin(), oldest.samples.begin() + discarded * channels);
			_frames -= discarded;
			if(oldest.samples.empty()) _chunks.erase(_chunks.begin());
		}

		_chunks.push_back({sampleRate, channels, CaptureNow(), std::vector<float>(samples + (size_t(frames) - retainedFrames) * channels, samples + size_t(frames) * channels)});
		_frames += retainedFrames;
	}

	bool SARelayVoice::CaptureState::Drain(std::vector<Chunk> &chunks)
	{
		LockGuard<Lockable> lock(_lock);
		chunks.swap(_chunks);
		_frames = 0;
		const bool formatChanged = _formatChanged;
		_formatChanged = false;
		return formatChanged;
	}

	double SARelayVoice::CaptureNow()
	{
		return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	SARelayVoice::CaptureCallback SARelayVoice::CreateCaptureCallback(std::shared_ptr<CaptureState> capture)
	{
		return [capture = std::move(capture)](uint32 sampleRate, uint32 channels, uint32 frames, const float *samples) {
			if(capture) capture->Enqueue(sampleRate, channels, frames, samples);
		};
	}

	void SARelayVoice::DrainCapture()
	{
		std::vector<CaptureState::Chunk> chunks;

		// Reset on format changes even if all new chunks are stale.
		if(_capture->Drain(chunks)) ResetEncoder();

		for(const auto &chunk : chunks)
			ProcessPCM(chunk.samples.data(), chunk.samples.size() / chunk.channels, chunk.sampleRate, chunk.channels, chunk.capturedAt);
	}

	SARelayVoice::SARelayVoice(SlinAir::RelayHttp &http, const String *httpsBaseURL, std::shared_ptr<CaptureState> capture) :
		SARelayClient(http, httpsBaseURL),
		_capture(capture ? std::move(capture) : std::make_shared<CaptureState>())
	{
		int error = OPUS_OK;
		_encoder = opus_encoder_create(SampleRate, 1, OPUS_APPLICATION_VOIP, &error);
		if(_encoder)
		{
			opus_encoder_ctl(_encoder, OPUS_SET_BITRATE(24000));
			opus_encoder_ctl(_encoder, OPUS_SET_COMPLEXITY(5));
			opus_encoder_ctl(_encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
			opus_encoder_ctl(_encoder, OPUS_SET_DTX(1));
		}
	}

	SARelayVoice::~SARelayVoice()
	{
		_capture->SetEnabled(false);
		ResetAudio();
		if(_encoder) opus_encoder_destroy(_encoder);
	}

	void SARelayVoice::Leave()
	{
		_capture->SetEnabled(false);
		ResetAudio();
		SARelayClient::Leave();
	}

	void SARelayVoice::ResetAudio()
	{
		for(auto &entry : _speakers) opus_decoder_destroy(entry.second.decoder);
		_speakers.clear();

		// Auth renewal can keep the UDP session. Keep sequence numbers running.
		ResetCapture();
	}

	void SARelayVoice::SetCaptureEnabled(bool enabled)
	{
		if(_capture->SetEnabled(enabled)) ResetEncoder();
	}

	void SARelayVoice::ResetCapture()
	{
		_capture->Reset();
		ResetEncoder();
	}

	void SARelayVoice::ResetEncoder()
	{
		_inputCount = _olderInputSamples = 0;
		_oldestInputCapturedAt = _newerInputCapturedAt = 0.0;
		_inputRate = _inputChannels = 0;
		_hasPreviousSample = false;
		_resamplePosition = 0.0;
		_nextSend = _time;
		if(_encoder) opus_encoder_ctl(_encoder, OPUS_RESET_STATE);
	}

	void SARelayVoice::HandleState(State state)
	{
		if(state != State::Connected) ResetAudio();
	}

	void SARelayVoice::HandleRoster(const Member *members, size_t count)
	{
		for(auto iterator = _speakers.begin(); iterator != _speakers.end();)
		{
			bool present = false;
			for(size_t i = 0; i < count; i++)
				if(members[i].sessionId == iterator->first && !strcmp(members[i].playerId, iterator->second.member.playerId) && members[i].spectator == iterator->second.member.spectator) present = true;
			if(present)
			{
				++iterator;
				continue;
			}

			opus_decoder_destroy(iterator->second.decoder);
			iterator = _speakers.erase(iterator);
		}
	}

	void SARelayVoice::ExpireInput(double now)
	{
		// Drop pending input and interpolation history together.
		if((_inputCount || _hasPreviousSample) && now - _oldestInputCapturedAt > StaleCaptureDuration) ResetEncoder();
	}

	void SARelayVoice::DiscardInput(size_t count)
	{
		_inputCount -= count;
		memmove(_input, _input + count, _inputCount * sizeof(float));
		if(_olderInputSamples)
		{
			_olderInputSamples -= std::min(count, _olderInputSamples);
			if(!_olderInputSamples) _oldestInputCapturedAt = _newerInputCapturedAt;
		}
	}

	void SARelayVoice::QueueSample(float sample)
	{
		// Keep only the newest 60 ms of capture.
		if(_inputCount == CaptureFrames) DiscardInput(FrameSamples);
		_input[_inputCount++] = sample;
	}

	void SARelayVoice::SubmitPCM(const float *samples, size_t frames, uint32 sampleRate, uint32 channels)
	{
		if(frames > sampleRate / 5) return;
		_capture->Enqueue(sampleRate, channels, static_cast<uint32>(frames), samples);
	}

	void SARelayVoice::ProcessPCM(const float *samples, size_t frames, uint32 sampleRate, uint32 channels, double capturedAt)
	{
		if(!_encoder || CaptureNow() - capturedAt > StaleCaptureDuration) return;

		if(_inputRate != sampleRate || _inputChannels != channels)
		{
			ResetEncoder();
			_inputRate = sampleRate;
			_inputChannels = channels;
		}

		const double step = static_cast<double>(sampleRate) / static_cast<double>(SampleRate);
		for(size_t frame = 0; frame < frames; frame++)
		{
			float sample = 0.0f;
			for(uint32 channel = 0; channel < channels; channel++)
			{
				float value = samples[frame * channels + channel];
				if(!std::isfinite(value)) value = 0.0f;
				sample += std::max(-1.0f, std::min(1.0f, value));
			}

			sample /= channels;
			if(!_hasPreviousSample)
			{
				_previousSample = sample;
				_oldestInputCapturedAt = capturedAt;
				_hasPreviousSample = true;
				continue;
			}

			while(_resamplePosition < 1.0)
			{
				QueueSample(_previousSample + (sample - _previousSample) * _resamplePosition);
				_resamplePosition += step;
			}
			_resamplePosition -= 1.0;
			_previousSample = sample;

			if(frame == 0 && !_olderInputSamples)
			{
				// The first interval can still interpolate the old sample.
				_olderInputSamples = _inputCount;
				_newerInputCapturedAt = capturedAt;
				if(!_olderInputSamples) _oldestInputCapturedAt = capturedAt;
			}
		}
	}

	void SARelayVoice::HandlePacket(uint64 sessionID, const char *playerID, uint16 channel, const void *bytes, size_t size)
	{
		if(channel)
		{
			SARelayClient::HandlePacket(sessionID, playerID, channel, bytes, size);
			return;
		}

		const uint8 *data = static_cast<const uint8 *>(bytes);
		if(size <= PacketHeaderBytes || size > MaxPacketBytes + PacketHeaderBytes || data[0] != PacketVersion ||
			opus_packet_get_nb_samples(data + PacketHeaderBytes, static_cast<opus_int32>(size - PacketHeaderBytes), SampleRate) != FrameSamples) return;

		const uint32 sequence = (static_cast<uint32>(data[1]) << 24) | (static_cast<uint32>(data[2]) << 16) | (static_cast<uint32>(data[3]) << 8) | data[4];
		auto iterator = _speakers.find(sessionID);
		if(iterator == _speakers.end())
		{
			if(_speakers.size() >= MaxMembers) return;

			int error = OPUS_OK;
			OpusDecoder *decoder = opus_decoder_create(SampleRate, 1, &error);
			if(!decoder) return;

			Speaker speaker;
			speaker.decoder = decoder;
			const Member *members = nullptr;
			const size_t count = GetRoster(members);
			const Member *member = nullptr;
			for(size_t i = 0; i < count; i++)
				if(members[i].sessionId == sessionID && !strcmp(members[i].playerId, playerID)) member = &members[i];
			if(!member)
			{
				opus_decoder_destroy(decoder);
				return;
			}
			speaker.member = *member;
			iterator = _speakers.emplace(sessionID, std::move(speaker)).first;
		}

		Speaker &speaker = iterator->second;
		if(speaker.received && static_cast<int32>(sequence - speaker.sequence) < 0) return;
		speaker.received = true;
		if(!speaker.playing || sequence - speaker.sequence >= JitterPackets)
		{
			for(auto &packet : speaker.packets) packet.size = 0;
			opus_decoder_ctl(speaker.decoder, OPUS_RESET_STATE);
			speaker.sequence = sequence;
			speaker.nextPlayback = _time + FrameDuration * StartupPackets;
			speaker.missing = 0;
			speaker.playing = true;
		}

		Packet &packet = speaker.packets[sequence % JitterPackets];
		if(packet.size && packet.sequence == sequence) return;
		packet.sequence = sequence;
		packet.size = size - PacketHeaderBytes;
		memcpy(packet.bytes, data + PacketHeaderBytes, packet.size);
	}

	void SARelayVoice::PlaySpeaker(Speaker &speaker)
	{
		if(!speaker.playing || _time < speaker.nextPlayback) return;

		// Resume at the newest packet after a stall.
		// Avoid bursts of old audio and limit missing audio concealment.
		if(_time - speaker.nextPlayback > CaptureDuration)
		{
			uint32 newest = speaker.sequence;
			for(const auto &packet : speaker.packets)
				if(packet.size && static_cast<int32>(packet.sequence - newest) > 0) newest = packet.sequence;
			for(auto &packet : speaker.packets)
				if(packet.sequence != newest) packet.size = 0;
			speaker.sequence = newest;
			speaker.nextPlayback = _time;
			opus_decoder_ctl(speaker.decoder, OPUS_RESET_STATE);
		}

		Packet &packet = speaker.packets[speaker.sequence % JitterPackets];
		const bool available = packet.size && packet.sequence == speaker.sequence;
		if(!available && speaker.missing >= MaxMissingPackets)
		{
			const Packet *next = nullptr;
			for(const auto &queued : speaker.packets)
				if(queued.size && static_cast<int32>(queued.sequence - speaker.sequence) > 0 &&
					(!next || static_cast<int32>(queued.sequence - next->sequence) < 0)) next = &queued;

			speaker.playing = next != nullptr;
			if(next)
			{
				opus_decoder_ctl(speaker.decoder, OPUS_RESET_STATE);
				speaker.sequence = next->sequence;
				speaker.nextPlayback = _time + FrameDuration * StartupPackets;
				speaker.missing = 0;
			}

			return;
		}

		int16 output[FrameSamples];
		const int decoded = opus_decode(speaker.decoder, available ? packet.bytes : nullptr, available ? static_cast<opus_int32>(packet.size) : 0, output, FrameSamples, 0);
		if(available) packet.size = 0;
		speaker.missing = available ? 0 : speaker.missing + 1;
		speaker.sequence++;
		speaker.nextPlayback += FrameDuration;
		if(decoded == FrameSamples && _playbackCallback) _playbackCallback(speaker.member, output, FrameSamples);
	}

	void SARelayVoice::Update(double monotonicSeconds)
	{
		if(!std::isfinite(monotonicSeconds) || monotonicSeconds < _time) return;

		_time = monotonicSeconds;
		SARelayClient::Update(monotonicSeconds);
		if(GetState() != State::Connected) return;

		ExpireInput(CaptureNow());
		DrainCapture();
		if(_time - _nextSend > CaptureDuration) _nextSend = _time;
		for(size_t sent = 0; _encoder && _inputCount >= FrameSamples && _time >= _nextSend && sent < CatchupPackets; sent++)
		{
			uint8 packet[MaxPacketBytes + PacketHeaderBytes];
			packet[0] = PacketVersion;
			packet[1] = static_cast<uint8>(_sequence >> 24);
			packet[2] = static_cast<uint8>(_sequence >> 16);
			packet[3] = static_cast<uint8>(_sequence >> 8);
			packet[4] = static_cast<uint8>(_sequence++);
			const int size = opus_encode_float(_encoder, _input, FrameSamples, packet + PacketHeaderBytes, MaxPacketBytes);
			DiscardInput(FrameSamples);
			_nextSend += FrameDuration;
			if(size > 2) _client.Broadcast(0, packet, size + PacketHeaderBytes);
		}

		for(auto &entry : _speakers)
			for(size_t played = 0; entry.second.playing && _time >= entry.second.nextPlayback && played < CatchupPackets; played++)
				PlaySpeaker(entry.second);
	}
}
