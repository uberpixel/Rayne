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
		RAAPI void SetRange(RN::Vector2 minMaxRange);
		RAAPI void SetSelfdestruct(bool selfdestruct);
		RAAPI void SetRolloffModel(DistanceRolloffModel rolloffModel);
		RAAPI void SetChannel(uint8 channel);

		void Update(float delta) override;
		RAAPI bool Update(double frameLength, uint32 sampleCount, float **outputBuffer, uint8 channelCount = 1);
		void Update();
		void DidUpdate(SceneNode::ChangeSet changeSet) override;

		bool IsPositional() const { return _isPositional; }

		bool IsPlaying() const { return _cachedIsPlaying.load(std::memory_order_relaxed); }
		bool IsRepeating() const { return _isRepeating.load(std::memory_order_relaxed); }
		RAAPI bool HasEnded() const;

		RAAPI float GetVolume() const { return _volume.load(std::memory_order_relaxed); }

		RN::Vector2 GetRange() const { return _minMaxRange; }
		ResonanceAudioSampler *GetSampler() const { return _sampler; }

	private:
		void UpdateDistanceModel();
		void SetSpatialPosition(const Vector3 &position);
		static AudioAsset *GetNullAssetSentinel() { return reinterpret_cast<AudioAsset *>(static_cast<uintptr_t>(1)); }
		void ConsumePendingState();
		bool ProcessPendingActions();

		ResonanceAudioWorld *_world; // The world is not retained and clears this pointer after callbacks stop.
		bool _isRegisteredInWorld;
		std::atomic<uint64> _playbackRevision {1};
		std::atomic<uint64> _selfdestructRequest {0};
		std::atomic<uint32> _ringUnderruns {0};
		float _ringDiagnosticsElapsed = 0.0f;

		std::atomic<uint8> _channel;
		ResonanceAudioSampler *_sampler;

		int _sourceID;

		bool _wantsIndirectSound;
		bool _isPositional;

		std::atomic<bool> _isSelfdestructing;
		std::atomic<bool> _isRepeating;

		std::atomic<float> _volume;
		std::atomic<float> _pitch;

		// Updated from ResonanceAudioWorld on the main thread; consumed on the audio thread.
		// This mirrors OpenAL's Doppler behaviour by modulating the sampler pitch.
		std::atomic<float> _dopplerPitchMultiplier;
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
		uint32 _currentSpatialSequence;
		int32 _fadeSamples; // >0 fade-in, <0 fade-out, 0 none
		uint32_t _controlBits;
		PendingAction _finalAction;
		double _nextSeekTime;
		AudioAsset *_nextAsset;

		//Used to sync between threads
		std::atomic<uint32> _spatialSequence;
		std::atomic<float> _spatialPositionX;
		std::atomic<float> _spatialPositionY;
		std::atomic<float> _spatialPositionZ;
		std::atomic<double> _pendingSeekTime;
		std::atomic<AudioAsset*> _pendingAsset;
		std::atomic<PendingAction> _pendingFinalAction;

		// Cached on audio thread to use on other threads
		std::atomic<bool> _cachedHasAsset;
		std::atomic<double> _cachedTotalTime;
		std::atomic<bool> _cachedIsPlaying;
		std::atomic<double> _cachedCurrentTime;

		RNDeclareMetaAPI(ResonanceAudioSource, RAAPI)
	};
} // namespace RN

#endif /* defined(__RAYNE_ResonanceAudioSOURCE_H_) */
