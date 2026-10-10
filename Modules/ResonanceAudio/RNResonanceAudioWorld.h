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
		void AddAudioSource(ResonanceAudioSource *source);
		void RemoveAudioSource(ResonanceAudioSource *source);
		void PublishAudioSourcesSnapshot(const std::vector<ResonanceAudioSource *> &sources);
		Array *AcquireAudioSourcesSnapshot();

		static ResonanceAudioWorld *_instance;

		ResonanceAudioSystem *_audioSystem;

		std::vector<ResonanceAudioSource *> _audioSources;
		bool _audioSourcesSnapshotDirty = false;
		Array *_audioSourcesSnapshots[3];
		// SPSC ownership: output callback owns front, scene update owns back.
		// Bit 2 marks a complete pending publication; exchange transfers ownership.
		uint32 _audioSourcesSnapshotFront = 0;
		std::atomic<uint32> _audioSourcesSnapshotPending {1};
		uint32 _audioSourcesSnapshotBack = 2;

		std::atomic<float> _worldMasterVolume;

		std::function<void(PositionType, Vector3, float &)> _raycastCallback;

		RNDeclareMetaAPI(ResonanceAudioWorld, RAAPI)
	};
} // namespace RN

#endif /* defined(__RAYNE_ResonanceAudioWORLD_H_) */
