//
//  RNResonanceAudioListenerContext.h
//  Rayne-ResonanceAudio
//
//  Copyright 2026 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RESONANCEAUDIO_LISTENERCONTEXT_H_
#define __RAYNE_RESONANCEAUDIO_LISTENERCONTEXT_H_

#include "RNResonanceAudio.h"

namespace vraudio
{
	class ResonanceAudioApi;
}

namespace RN
{
	class SceneNode;
	class ResonanceAudioWorld;

	struct ResonanceAudioListenerState
	{
		PositionType position;
		Vector3 velocity;
		Quaternion rotation;
		bool isValid = false;
	};

	class ResonanceAudioListenerContext : public Object
	{
		friend class ResonanceAudioSystem;

	public:
		RAAPI ResonanceAudioListenerContext(ResonanceAudioWorld *world, uint32 channelCount, uint32 frameSize, uint32 sampleRate);
		RAAPI ~ResonanceAudioListenerContext() override;

		RAAPI void SetMasterVolume(float volume);
		RAAPI void SetWetVolume(float volume);
		RAAPI void SetDryVolume(float volume);
		float GetMasterVolume() const { return _listenerMasterVolume.load(std::memory_order_relaxed); }
		float GetWetVolume() const { return _wetVolume.load(std::memory_order_relaxed); }
		float GetDryVolume() const { return _dryVolume.load(std::memory_order_relaxed); }

		RAAPI void SetDopplerEffect(float factor, float speedOfSound = 343.3f);
		RAAPI void SetDopplerVelocitySmoothing(float oldVelocityWeight = 0.95f);
		float GetDopplerFactor() const { return _dopplerFactor; }
		float GetDopplerSpeedOfSound() const { return _dopplerSpeedOfSound; }
		float GetDopplerVelocitySmoothing() const { return _dopplerVelocitySmoothing; }

		RAAPI void SetSimpleRoom(const PositionType &position, Vector3 dimensions, float reflectionConstant, ResonanceAudioMaterial left, ResonanceAudioMaterial right, ResonanceAudioMaterial bottom, ResonanceAudioMaterial top, ResonanceAudioMaterial front, ResonanceAudioMaterial back);
		RAAPI void SetSimpleRoomEnabled(bool enabled);

		RAAPI void SetInputSamplesCallback(std::function<void(uint32 /*sampleRate*/, uint32 /*channelCount*/, uint32 /*frameCount*/, const float * /*frames*/)> inputSamplesCallback);

		RAAPI void SetListener(SceneNode *listener);
		SceneNode *GetListener() const { return _listener; }

		ResonanceAudioListenerState GetListenerState() const;

		RAAPI void Update(float delta);

		void RenderAudio(void *outputBuffer, const void *inputBuffer, uint32 sampleRate, uint32 channelCount, uint32 frameCount, uint32 status);

		vraudio::ResonanceAudioApi *GetAudioAPI() const { return _audioAPI; }
		float *GetSharedFrameData() const { return _sharedFrameData; }

	private:
		ResonanceAudioWorld *_world;
		SceneNode *_listener;
		PositionType _oldPosition;

		uint32 _frameSize;
		uint32 _channelCount;
		float *_sharedFrameData;

		RecursiveLockable _inputSamplesCallbackLock;
		std::function<void(uint32, uint32, uint32, const float *)> _inputSamplesCallback;

		std::atomic<float> _listenerMasterVolume;
		std::atomic<float> _wetVolume;
		std::atomic<float> _dryVolume;

		float _dopplerFactor;
		float _dopplerSpeedOfSound;
		float _dopplerVelocitySmoothing; // old velocity weight

		bool _roomEnabled;
		bool _roomDirty;
		PositionType _roomPosition;
		Vector3 _roomDimensions;
		float _roomReflectionConstant;
		ResonanceAudioMaterial _roomMaterials[6];

		ResonanceAudioListenerState _stateBuffers[2] = {};
		std::atomic<uint32> _stateIndex {0};

		vraudio::ResonanceAudioApi *_audioAPI;

		RNDeclareMetaAPI(ResonanceAudioListenerContext, RAAPI)
	};
} // namespace RN

#endif // __RAYNE_RESONANCEAUDIO_LISTENERCONTEXT_H_
