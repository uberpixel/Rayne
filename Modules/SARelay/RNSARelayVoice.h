#ifndef __RAYNE_SARELAYVOICE_H_
#define __RAYNE_SARELAYVOICE_H_

#include "RNSARelayClient.h"

struct OpusEncoder;
struct OpusDecoder;

namespace RN
{
	class SARelayVoice : public SARelayClient
	{
	public:
		static constexpr uint32 SampleRate = 48000;
		static constexpr size_t FrameSamples = 960;

		using CaptureCallback = std::function<void(uint32, uint32, uint32, const float *)>;

		class CaptureState
		{
		public:
			CaptureState() = default;

		private:
			friend class SARelayVoice;

			struct Chunk
			{
				uint32 sampleRate, channels;
				double capturedAt;
				std::vector<float> samples;
			};

			bool SetEnabled(bool enabled);
			void Reset();
			void Enqueue(uint32 sampleRate, uint32 channels, uint32 frames, const float *samples);
			bool Drain(std::vector<Chunk> &chunks);

			Lockable _lock;
			bool _enabled = false;
			size_t _frames = 0;
			uint32 _sampleRate = 0, _channels = 0;
			bool _formatChanged = false;
			std::vector<Chunk> _chunks;
		};

		// Share capture state across relay creation or replacement.
		// Each state has one live relay owner at most. Capture starts disabled.
		// Callbacks stay safe after the relay or its owner dies.
		SARELAYAPI static CaptureCallback CreateCaptureCallback(std::shared_ptr<CaptureState> capture);

		SARELAYAPI SARelayVoice(SlinAir::RelayHttp &http, const String *httpsBaseURL, std::shared_ptr<CaptureState> capture = nullptr);
		SARELAYAPI ~SARelayVoice() override;
		SARELAYAPI void Leave() override;
		SARELAYAPI void Update(double monotonicSeconds) override;

		// Queue interleaved float PCM on the owner thread. Update resamples it.
		// Capture threads use CreateCaptureCallback.
		// Input is 8 to 192 kHz, 1 to 8 channels, up to 200 ms.
		SARELAYAPI void SubmitPCM(const float *samples, size_t frames, uint32 sampleRate, uint32 channels);

		// Call on the owner thread.
		// Changes discard queued capture and reset the encoder.
		SARELAYAPI void SetCaptureEnabled(bool enabled);

		// Clear unsent capture and keep the send gate. Playback stays active.
		SARELAYAPI void ResetCapture();

		// Borrowed 48 kHz mono int16 PCM: exactly 960 frames per callback.
		void SetPlaybackCallback(std::function<void(const Member &, const int16 *, size_t)> callback) { _playbackCallback = std::move(callback); }

	protected:
		void HandleState(State state) override;
		void HandleRoster(const Member *members, size_t count) override;
		void HandlePacket(uint64 sessionID, const char *playerID, uint16 channel, const void *bytes, size_t size) override;

	private:
		static constexpr size_t CatchupPackets = 3;
		static constexpr size_t CaptureFrames = FrameSamples * CatchupPackets;
		static constexpr size_t JitterPackets = 6;
		static constexpr size_t StartupPackets = 2;
		static constexpr size_t MaxMissingPackets = 2;
		static constexpr size_t MaxPacketBytes = 400;
		static constexpr size_t PacketHeaderBytes = 5;
		static constexpr uint8 PacketVersion = 1;
		static constexpr double FrameDuration = static_cast<double>(FrameSamples) / SampleRate;
		static constexpr double CaptureDuration = FrameDuration * CatchupPackets;
		static constexpr double StaleCaptureDuration = 0.2;

		struct Packet
		{
			uint32 sequence = 0;
			size_t size = 0;
			uint8 bytes[MaxPacketBytes];
		};

		struct Speaker
		{
			OpusDecoder *decoder = nullptr;
			Member member = {};
			Packet packets[JitterPackets];
			uint32 sequence = 0;
			double nextPlayback = 0.0;
			uint32 missing = 0;
			bool playing = false, received = false;
		};

		void ResetAudio();
		void ResetEncoder();
		static double CaptureNow();
		void ExpireInput(double now);
		void DiscardInput(size_t count);
		void QueueSample(float sample);
		void ProcessPCM(const float *samples, size_t frames, uint32 sampleRate, uint32 channels, double capturedAt);
		void PlaySpeaker(Speaker &speaker);
		void DrainCapture();

		std::shared_ptr<CaptureState> _capture;
		OpusEncoder *_encoder;
		std::map<uint64, Speaker> _speakers;
		float _input[CaptureFrames];
		size_t _inputCount = 0;
		uint32 _inputRate = 0, _inputChannels = 0, _sequence = 0;
		float _previousSample = 0.0f;

		// After the older prefix is consumed, the newer age covers all pending input.
		double _oldestInputCapturedAt = 0.0, _newerInputCapturedAt = 0.0;
		size_t _olderInputSamples = 0;
		double _resamplePosition = 0.0, _time = 0.0, _nextSend = 0.0;
		bool _hasPreviousSample = false;
		std::function<void(const Member &, const int16 *, size_t)> _playbackCallback;

		RNDeclareMetaAPI(SARelayVoice, SARELAYAPI)
	};
}

#endif
