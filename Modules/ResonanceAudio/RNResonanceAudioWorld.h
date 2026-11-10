//
//  RNResonanceAudioWorld.h
//  Rayne-ResonanceAudio
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ResonanceAudioWORLD_H_
#define __RAYNE_ResonanceAudioWORLD_H_

#include "RNResonanceAudio.h"
#include "RNResonanceAudioSource.h"
#include "RNResonanceAudioSystem.h"
#include "RNResonanceAudioListenerContext.h"

namespace RN
{
	class ResonanceAudioWorld : public SceneAttachment
	{
	public:
		friend class ResonanceAudioSource;
		friend class ResonanceAudioSystem;
		friend class ResonanceAudioListenerContext;

		enum MicrophonePermissionState
		{
			MicrophonePermissionStateAuthorized,
			MicrophonePermissionStateNotDetermined,
			MicrophonePermissionStateForbidden
		};

		RAAPI static ResonanceAudioWorld *GetInstance();

		RAAPI ResonanceAudioWorld(ResonanceAudioSystem *audioSystem);
		RAAPI ~ResonanceAudioWorld() override;

		ResonanceAudioSystem *GetAudioSystem() const { return _audioSystem; }
		ResonanceAudioListenerContext *GetListenerContext() const { return _audioSystem ? _audioSystem->GetListenerContext() : nullptr; }
		vraudio::ResonanceAudioApi *GetAudioAPI() const { ResonanceAudioListenerContext *ctx = GetListenerContext(); return ctx ? ctx->GetAudioAPI() : nullptr; }

		RAAPI void SetListener(SceneNode *listener);
		// Scene thread only. A successful start begins on an output block boundary
		// after the next scene update publishes the listener pose.
		// Stop waits for the last callback to return. Keep the callback nonblocking
		// and nonthrowing, with no calls into the audio module.
		RAAPI bool StartRecordingListener(SceneNode *listener, ResonanceAudioRecordingCallback callback);
		RAAPI void StopRecordingListener();
		SceneNode *GetListener() const;

		RAAPI void SetMasterVolume(float volume);
		RAAPI void SetWetVolume(float volume);
		RAAPI void SetDryVolume(float volume);

		RAAPI void SetDopplerEffect(float factor, float speedOfSound = 343.3f);
		RAAPI void SetDopplerVelocitySmoothing(float oldVelocityWeight = 0.95f);

		ResonanceAudioListenerState GetListenerState() const;

		RAAPI ResonanceAudioSource *PlaySound(AudioAsset *resource) const;
		RAAPI ResonanceAudioSource *PlaySound(AudioAsset *resource, const PositionType &position) const;

		RAAPI void SetRaycastCallback(const std::function<void(PositionType, Vector3, float &)> &raycastCallback);
		RAAPI void SetSimpleRoom(const PositionType &position, Vector3 dimensions, float reflectionConstant, ResonanceAudioMaterial left, ResonanceAudioMaterial right, ResonanceAudioMaterial bottom, ResonanceAudioMaterial top, ResonanceAudioMaterial front, ResonanceAudioMaterial back);
		RAAPI void SetSimpleRoomEnabled(bool enabled);

		RAAPI void SetInputSamplesCallback(std::function<void(uint32 /*sampleRate*/, uint32 /*channelCount*/, uint32 /*frameCount*/, const float * /*frames*/)> inputSamplesCallback);

		RAAPI static void RequestMicrophonePermission();
		RAAPI static MicrophonePermissionState GetMicrophonePermissionState();

	protected:
		void Update(float delta) override;

	private:
		struct AudioSourcesSnapshot
		{
			Array *sources;
			uint64 sourcesRevision = 0;
			uint64 spatialRevision = 0;
			Quaternion rotation[2];
			std::vector<ResonanceAudioSource::SpatialState> sourceState[2];
		};

		void AddAudioSource(ResonanceAudioSource *source);
		void RemoveAudioSource(ResonanceAudioSource *source);
		void PublishAudioSourcesSnapshot(float delta);
		void RenderOutput(ResonanceAudioListenerContext &primary, float *output, uint32 frameCount, uint32 sampleRate, uint32 channelCount, ResonanceAudioListenerContext *recording);
		void SetListenerMixSettings(const std::function<void(ResonanceAudioListenerContext *)> &apply);
		const AudioSourcesSnapshot &AcquireAudioSourcesSnapshot();

		static RecursiveLockable _sourceLifetimeLock;
		static ResonanceAudioWorld *_instance;

		ResonanceAudioSystem *_audioSystem;

		std::vector<ResonanceAudioSource *> _allAudioSources; // Tracks all sources, including detached sources, without retaining them.
		std::vector<ResonanceAudioSource *> _audioSources;
		uint64 _audioSourcesRevision = 0;
		AudioSourcesSnapshot _audioSourcesSnapshots[3];
		uint64 _spatialRevision = 0;
		ResonanceAudioListenerContext *_recordingContext = nullptr; // Scene thread only.
		bool _recordingPending = false;
		// SPSC ownership: output callback owns front, scene update owns back.
		// Bit 2 marks a complete pending publication; exchange transfers ownership.
		uint32 _audioSourcesSnapshotFront = 0;
		std::atomic<uint32> _audioSourcesSnapshotPending {1};
		uint32 _audioSourcesSnapshotBack = 2;

		std::atomic<bool> _hasRetiredAudioAssets {false};

		std::atomic<float> _worldMasterVolume;

		std::function<void(PositionType, Vector3, float &)> _raycastCallback;

		RNDeclareMetaAPI(ResonanceAudioWorld, RAAPI)
	};
} // namespace RN

#endif /* defined(__RAYNE_ResonanceAudioWORLD_H_) */
