//
//  RNResonanceAudioSource.h
//  Rayne-ResonanceAudio
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ResonanceAudioSOURCE_H_
#define __RAYNE_ResonanceAudioSOURCE_H_

#include "RNResonanceAudio.h"

namespace RN
{
	class ResonanceAudioSampler;
	class ResonanceAudioWorld;
	class ResonanceAudioListenerContext;
	struct ResonanceAudioSourceInternals;
	class ResonanceAudioSource : public SceneNode
	{
	public:
		friend class ResonanceAudioWorld;
		friend class ResonanceAudioListenerContext;

		enum class PendingAction
		{
			None,
			Stop,
			Pause,
			Play
		};

		enum class DistanceRolloffModel
		{
			Logarithmic,
			Linear,
			Inverse,
			None
		};

		enum ControlBits : uint32_t
		{
			kWantFadeOut     = 1u << 0,
			kWantFadeIn      = 1u << 1,
			kWantSeek        = 1u << 2,
			kWantAssetChange = 1u << 3
		};

		RAAPI ResonanceAudioSource(AudioAsset *asset = nullptr, bool wantsIndirectSound = true, bool isPositional = true);
		RAAPI ~ResonanceAudioSource() override;

		RAAPI void Play();
		RAAPI void Stop();
		RAAPI void Pause();
		RAAPI void Seek(double time);

		RAAPI void SetAudioAsset(AudioAsset *asset);

		RAAPI void SetRepeat(bool repeat);
		RAAPI void SetCurrentDistanceAttenuationValue(float attentuation);
		RAAPI void SetPitch(float pitch);
		RAAPI void SetVolume(float volume);
		// Overrides the recording listener's volume for positional sources. A negative value uses SetVolume again.
		RAAPI void SetRecordingSpatialVolume(float volume);
		RAAPI void SetRange(RN::Vector2 minMaxRange);
		RAAPI void SetSelfdestruct(bool selfdestruct);
		RAAPI void SetRolloffModel(DistanceRolloffModel rolloffModel);
		RAAPI void SetChannel(uint8 channel);

		void Update(float delta) override;
		RAAPI bool Update(double frameLength, uint32 sampleCount, float **outputBuffer, uint8 channelCount = 1);
		void DidUpdate(SceneNode::ChangeSet changeSet) override;

		bool IsPositional() const { return _isPositional; }

		bool IsPlaying() const { return _cachedIsPlaying.load(std::memory_order_relaxed); }
		// Reports the recording cursor's latest playback state for a positional source.
		bool IsRecordingPlaying() const { return _cachedIsRecordingPlaying.load(std::memory_order_relaxed); }
		bool IsRepeating() const { return _isRepeating.load(std::memory_order_relaxed); }
		RAAPI bool HasEnded() const;

		RAAPI float GetVolume() const { return _volume.load(std::memory_order_relaxed); }

		RN::Vector2 GetRange() const { return _minMaxRange; }
		ResonanceAudioSampler *GetSampler() const { return _sampler; }

	private:
		struct SpatialState
		{
			Vector3 position;
			float doppler = 1.0f;
			float roomGain = 0.0f;
			float occlusion = 0.0f;
			bool valid = false;
			bool roomEnabled = false;
		};

		struct RecordingPlayback
		{
			double time = 0.0;
			float pitch = 1.0f;
			bool initialized = false;
			bool playing = false;
		};

		struct RenderedSamples
		{
			bool primary = false;
			bool recording = false;
		};

		void UpdateDistanceModel(); // Called with the source lifetime lock held.
		void ApplyDistanceModel(ResonanceAudioListenerContext &context, int sourceID) const;
		void CreateRecordingSource(ResonanceAudioListenerContext *context);
		void RemoveRecordingSource(ResonanceAudioListenerContext *context);
		float GetRecordingSpatialVolume(float sharedVolume) const;
		void UpdateDopplerVelocity(const PositionType &position, float delta, float smoothing);
		float GetDopplerPitch(const ResonanceAudioListenerContext &context, const Vector3 &offset, const Vector3 &listenerVelocity) const;
		void ApplySpatialState(ResonanceAudioListenerContext &context, int sourceID, const SpatialState &state, bool &roomWasEnabled);
		void RenderPositional(ResonanceAudioListenerContext &primary, ResonanceAudioListenerContext *recording, const SpatialState &primaryState, const SpatialState *recordingState, bool primarySpatialChanged, bool recordingSpatialChanged, double frameLength, uint32 sampleCount);
		RenderedSamples RenderSamples(double frameLength, uint32 sampleCount, float *output, uint8 channelCount, float primaryDoppler = 1.0f, float *recordingOutput = nullptr, float recordingDoppler = 1.0f);
		void FinishRender(uint64 playbackRevision, bool recording);
		static AudioAsset *GetNullAssetSentinel() { return reinterpret_cast<AudioAsset *>(static_cast<uintptr_t>(1)); }
		void ConsumePendingState(bool recordingPlaying);
		void ProcessPendingActions(RecordingPlayback *recording, bool blockBoundary = true);
		void RetireAudioAsset(AudioAsset *asset);
		void ReleaseRetiredAudioAssets();

		ResonanceAudioWorld *_world; // The world is not retained and clears this pointer after callbacks stop.
		bool _isRegisteredInWorld;
		std::atomic<uint64> _playbackRevision {1};
		std::atomic<uint64> _selfdestructRequest {0};
		std::atomic<uint32> _ringUnderruns {0};
		float _ringDiagnosticsElapsed = 0.0f;

		std::atomic<uint8> _channel;
		ResonanceAudioSampler *_sampler;

		int _sourceID;
		int _recordingSourceID;
		RecordingPlayback _recordingPlayback;
		float _recordingVolume = -1.0f;
		bool _primaryRoomWasEnabled = false;
		bool _recordingRoomWasEnabled = false;
		float _manualDistanceAttenuation = 1.0f;

		bool _wantsIndirectSound;
		bool _isPositional;

		std::atomic<bool> _isSelfdestructing;
		std::atomic<bool> _isRepeating;

		std::atomic<float> _volume;
		std::atomic<float> _recordingSpatialVolume {-1.0f};
		std::atomic<float> _pitch;

		// The scene thread estimates velocity once for both listeners.
		PositionType _dopplerOldPosition;
		Vector3 _dopplerVelocity; // smoothed
		bool _dopplerInitialized;

		RN::Vector2 _minMaxRange;
		DistanceRolloffModel _rolloffModel;

		//Only used on audio thread
		bool _isPlaying;
		double _currentTime;
		float _currentPitch;
		float _currentVolume;
		int32 _fadeSamples; // >0 fade-in, <0 fade-out, 0 none
		uint32_t _controlBits;
		PendingAction _finalAction;
		double _nextSeekTime;
		AudioAsset *_nextAsset;

		// Output owns writes. The world update drains every source, including those outside the scene.
		static constexpr uint32 kAudioAssetRetirementCapacity = 8;
		AudioAsset *_retiredAudioAssets[kAudioAssetRetirementCapacity];
		std::atomic<uint32> _retiredAudioAssetWrite {0};
		std::atomic<uint32> _retiredAudioAssetRead {0};

		//Used to sync between threads
		std::atomic<double> _pendingSeekTime;
		std::atomic<AudioAsset*> _pendingAsset;
		std::atomic<PendingAction> _pendingFinalAction;

		// Cached on audio thread to use on other threads
		std::atomic<bool> _cachedHasAsset;
		std::atomic<double> _cachedTotalTime;
		std::atomic<bool> _cachedIsPlaying;
		std::atomic<bool> _cachedIsRecordingPlaying {false};
		std::atomic<double> _cachedCurrentTime;

		RNDeclareMetaAPI(ResonanceAudioSource, RAAPI)
	};
} // namespace RN

#endif /* defined(__RAYNE_ResonanceAudioSOURCE_H_) */
